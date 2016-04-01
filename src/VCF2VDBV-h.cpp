

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <omp.h>
#include <errno.h>
//#include "omp.h"


#define BUFFER_SIZE 512000
#define INITIAL_VT_LINE_WIDTH 5000
#define NO_LINES_PER_THREAD 512


void clearBuffer(unsigned char * buf, int &bufPos, int size)
{
    for (int i=0; i < size; i++)
    {
        buf[i] = 0;
    }
    bufPos = 0;
}

void flushBuffer(char * filename, unsigned char * buf, int &bufPos)
{
    int last = 0;
    if (bufPos%8 > 0)
        last = 1;
    FILE *file;
    file = fopen(filename, "ab");
    if (filename == NULL) {
        printf("Cannot open %s\n", filename);
        printf("The error is - %s\n", strerror(errno));
        exit(8);
    }

    fwrite(buf, 1, bufPos/8 + last, file);
    fclose(file);
}


void writeBits(char * filename, unsigned char * buf, int &bufPos, unsigned char newBits, int number) //number:1-8
{
    short written;
    int leftToPut = number;
    if(bufPos/8 == BUFFER_SIZE)
    {
        FILE *file;
        file = fopen(filename, "ab");
        if (filename == NULL) {
            printf("Cannot open %s\n", filename);
            printf("The error is - %s\n", strerror(errno));
            exit(8);
        }
        fclose(file);

        clearBuffer(buf, bufPos, BUFFER_SIZE);
    }
    int bitsToFillBuf = BUFFER_SIZE*8-bufPos;
    
    
    if(bitsToFillBuf < sizeof(unsigned char) && bitsToFillBuf <= leftToPut)
    {
        
        buf[bufPos/8] = buf[bufPos/8] | (newBits >> (unsigned char) (sizeof(unsigned char)*8-bitsToFillBuf));
        bufPos+=bitsToFillBuf;
        leftToPut = leftToPut - bitsToFillBuf;
        newBits = newBits << bitsToFillBuf;
        
        if(bufPos/8 == BUFFER_SIZE)
        {
            FILE *file;
            file = fopen(filename, "ab");
            if (filename == NULL) {
                printf("Cannot open %s\n", filename);
                printf("The error is - %s\n", strerror(errno));
                exit(8);
            }
            fclose(file);

            clearBuffer(buf, bufPos, BUFFER_SIZE);
        }
        
    }
    while (leftToPut >0)
    {
        buf[bufPos/8] =  buf[bufPos/8] | (newBits >> bufPos%8);
        
        written = ((8-bufPos%8) <leftToPut? (8-bufPos%8) : leftToPut);
        bufPos = bufPos + written;
        
        
        leftToPut = leftToPut - written;
        newBits = newBits << written;

    }

    
}



//input arg:  vcf  [optional: chr name]
int main (int argc, char * const argv[]) {
    
    char* p, * r;
    p = (char *) malloc (200 *sizeof(char));
    r = (char *) malloc (10 *sizeof(char));
    
    FILE * vcf;	//input file with variants
    FILE * out_vl;  // output file with variant database
    
    
    printf("input vcf: %s\n", argv[1]);
    vcf = fopen(argv[1], "r");
    setvbuf(vcf, NULL, _IOFBF, 1 << 22);
    if (vcf == NULL) {
        printf("Cannot open %s\n", argv[1]);
        printf("The error is - %s\n", strerror(errno));
        exit(8);
    }
    
    char * resultFileName,  *helpFileName;
    helpFileName = (char*) malloc (50 *sizeof(char));
    
    resultFileName = (char *) malloc ((strlen(argv[1])+ 20) * sizeof(char));
    resultFileName[0]='\0';
    strcat(resultFileName, argv[1]);
    strcat(resultFileName, ".vd");
    strcat(resultFileName, "\0");
    
    out_vl = fopen(resultFileName, "w");
    if (out_vl == NULL) {
        printf("Cannot open %s\n", resultFileName);
        printf("The error is - %s\n", strerror(errno));
        exit(8);
    }
    printf("Output variant database: %s\n", resultFileName);
    
    
    
    //chr name
    if(argc >= 3)
    {
        strcat(p , argv[2]);
    }
    else
    {
        p = argv[1];
        if (strrchr(p,'/') != NULL)
            p = strrchr(p,'/')+1;
        p = strtok( p, "." );
        p = strtok ( NULL, "." );
    }
    
	int  i,  j =0, total = 0;
    int writeno;
	
    
    
    
    unsigned long int vcfSize, logTreshold;
    size_t tmpPt;
    
    char  data[200];
    
    
	
    do {
        fscanf(vcf, "%s", data);
    }
    while (strncmp(data, "#CHROM", 6) != 0);
    
    tmpPt = ftell(vcf);
    fseek( vcf, 0, SEEK_END);
	vcfSize = ftell(vcf);
    fseek( vcf, tmpPt, SEEK_SET);
    logTreshold =  vcfSize/5;
    
	
    fscanf(vcf, "%s %s %s %s %s %s %s %s", data, data, data, data, data, data, data, data);
    
    int noGen;
    
    //count no genomes
    tmpPt = ftell(vcf);
    noGen = 0;
    while (getc(vcf) != '\n')
    {
        //fseek( vcf, -1, SEEK_CUR);
        fscanf(vcf, "%s", data);
        noGen ++;
    }
    fseek( vcf, tmpPt, SEEK_SET);
    
    int bufPos[noGen];
	unsigned char ** buf;
    buf = (unsigned char **) malloc (noGen * sizeof(unsigned char *));
    for (i=0; i < noGen; i++)
    {
        buf[i] = (unsigned char *) malloc (BUFFER_SIZE * sizeof(unsigned char));
        clearBuffer(buf[i], bufPos[i], BUFFER_SIZE);
    }
    
    
    
  //  FILE * resultSeq[noGen];
    
    char * bvName[noGen];
    
    char **genomes;
    genomes = (char **) malloc (noGen * sizeof(char *));
    for (i=0; i < noGen; i++)
	{
		genomes[i] = (char *) malloc (40 * sizeof(char));
	}
    
    i = 1;j=0;
    
    FILE * file;
    
    for (i=0; i < noGen; i++)
    {
        fscanf(vcf, "%s", genomes[i]);
        
        bvName[i] = (char*) malloc ((strlen(genomes[i])+30) * sizeof(char));
        helpFileName[0]='\0';
        strcat(helpFileName, genomes[i]);
        strcat(helpFileName, ".");
        strcat(helpFileName, p);
        strcat(helpFileName, "\0");
        
        
        bvName[i][0]='\0';
        strcat(bvName[i], helpFileName);
        strcat(bvName[i], ".bv");
        strcat(bvName[i], "\0");
        
        file = fopen(bvName[i], "wb");
        fclose(file);
            
        
    }
    
    
    int noThreads;
#pragma omp parallel
    {
        noThreads = omp_get_num_threads();
    }
    printf("Processing... no threads: %d\n", noThreads);
    
    char **line,  chl;
    int  l;
    
    int *bitsCalculated;
    bitsCalculated = (int *) malloc(noThreads * sizeof(int));
    if(bitsCalculated == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(9);
    }
    
    unsigned char *** tempBits;
    int line_max[NO_LINES_PER_THREAD*noThreads];
    
    int char_max[noThreads];
    
    line = (char** ) malloc(NO_LINES_PER_THREAD*noThreads*sizeof(char*));
    if(line == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(9);
    }
    for (i=0; i < NO_LINES_PER_THREAD*noThreads; i++)
    {
        line[i] = (char *) malloc (INITIAL_VT_LINE_WIDTH * sizeof(char));
        if(line[i] == NULL)
        {
            fprintf(stderr, "Out of memory\n");
            exit(9);
        }
        line_max[i] = INITIAL_VT_LINE_WIDTH;
    }
    
    tempBits = ( unsigned char ***) malloc(noThreads * sizeof(unsigned char**));
    if(tempBits == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(9);
    }
    
    for (i = 0; i < noThreads; i++)
    {
        tempBits[i] = (unsigned char**)malloc(noGen * sizeof(unsigned char*));
        if(tempBits[i] == NULL)
        {
            fprintf(stderr, "Out of memory\n");
            exit(9);
        }
        
        
        char_max[i] = 1+(NO_LINES_PER_THREAD-1)/8;
        
        for (j = 0; j < noGen; j++)
        {
            tempBits[i][j] = (unsigned char*)malloc(((NO_LINES_PER_THREAD-1)/8+1) * sizeof( unsigned char));
            if(tempBits[i][j] == NULL)
            {
                fprintf(stderr, "Out of memory\n");
                exit(9);
            }
        }
        
    }
    
    int * VLbufPos = (int*) malloc (noThreads * sizeof (int));
    char ** VLbuffer;
    VLbuffer = (char **) malloc (noThreads *sizeof(char*));
    int * buf_max;
    buf_max = (int *) malloc (noThreads * sizeof(int));
    
    
    char **chrom, **name, **refC, **altList, **alt, **filter, **info, **format, **qual, **genotype, **endPos;
    
    
    chrom = (char **) malloc (noThreads *sizeof(char*));
    name = (char **) malloc (noThreads *sizeof(char*));
    refC = (char **) malloc (noThreads *sizeof(char*));
    altList = (char **) malloc (noThreads *sizeof(char*));
    alt = (char **) malloc (noThreads *sizeof(char*));
    filter = (char **) malloc (noThreads *sizeof(char*));
    info = (char **) malloc (noThreads *sizeof(char*));
    format = (char **) malloc (noThreads *sizeof(char*));
    qual = (char **) malloc (noThreads *sizeof(char*));
    genotype = (char **) malloc (noThreads *sizeof(char*));
    endPos = (char **) malloc (noThreads *sizeof(char*));
    
    for (i = 0; i < noThreads; i++)
    {
        VLbuffer[i] = (char *) malloc (NO_LINES_PER_THREAD*100*sizeof(char));
        buf_max[i] = NO_LINES_PER_THREAD*100;
        
        
        chrom[i] = (char *) malloc (15 * sizeof(char));
        name[i] = (char *) malloc (1000 *sizeof(char));
        refC[i] = (char *) malloc (1000000 *sizeof(char));
        altList[i] = (char *) malloc (1000000 *sizeof(char));
        alt[i] = (char *) malloc (1000000  *sizeof(char));
        filter[i] = (char *) malloc (20 *sizeof(char));
        info[i] = (char *) malloc (1000 *sizeof(char));
        format[i] = (char *) malloc (100 *sizeof(char));
        qual[i] = (char *) malloc (20 *sizeof(char));
        genotype[i] = (char *) malloc (200 *sizeof(char));
        endPos[i] = (char *) malloc (25 *sizeof(char));
    }
    
    
    
    chl = getc(vcf); //gets '\n'
    
    // unsigned long writtenBytes;
    
    int move, lastLine;
    char *ptr_s;
    while (chl != EOF)
    {
        
        // printf("Aaa\n");
        if(ftell(vcf) > logTreshold)
        {
            printf("Processed %.f%% of the input vcf\n", (float)(logTreshold)/(float)(vcfSize)*100);
            logTreshold = logTreshold + vcfSize/5;
        }
        
        
        
        lastLine = NO_LINES_PER_THREAD * noThreads;
        for (i = 0; i < lastLine; i++)
        {
            move = 0;
            while(1)
            {
                
                if (fgets(line[i]+move, line_max[i]-move, vcf) && !feof(vcf)) {
                    ptr_s = strchr(line[i], '\n');
                    if (ptr_s) {
                        *ptr_s = '\0';
                        break;
                    }
                    else
                    {
                        line[i] = (char *) realloc(line[i], (line_max[i]*2)* sizeof(char));
                        move = line_max[i] - 1;
                        line_max[i] = line_max[i]*2;
                    }
                }
                else
                {
                    line[i][0] = '\n';
                    lastLine = i - 1;
                    break;
                }
            };
        }
#pragma omp parallel
        {
#pragma omp for
            for(int t = 0; t < noThreads; t++)
            {
                int pos,  currentPos = 0, currChar;
                int  gt_pos, noAlt, a, gt;
                
                char* pp, * rr;
                pp = new char[200];
                rr = new char[10];
                
                short currentLine;
                int mv, ptr;
                int j;
                long int shift;
                bool variantOK;
                char *strtokState1, *strtokState2;
                
                int returnValue;
                bool isInsertion;
                char * chr_ptr;
                int temp_int, delLen;
                
                currentPos = 0;
                bitsCalculated[t] = 0;
                
                char tempChar;
                for (int g = 0; g < noGen; g++)
                {
                    for (int b = 0; b < char_max[t]; b++)
                    {
                        tempChar = 0x00000000;
                        tempBits[t][g][b] = 0x00000000;
                    }
                }
                VLbufPos[t] = 0;
                
                for (j = 0; j < NO_LINES_PER_THREAD; j++)
                {
                    mv = 0, ptr = 0;
                    variantOK = true;
                    currentLine = NO_LINES_PER_THREAD*t + j;
                    if(currentLine > lastLine)
                    {
                        break;
                    }
                    sscanf(line[currentLine], "%s %d %s %s %s %s %s %s %s%n", chrom[t], &pos, name[t], refC[t], altList[t], qual[t], filter[t], info[t], format[t], &ptr);
                    if(ptr == 0)
                        break;
                    
                    pp = strtok_r(format[t], ":", &strtokState1);
                    gt_pos = 0;
                    while (strcmp(pp,"GT") != 0) {
                        pp = strtok_r(NULL, ":", &strtokState1);
                        gt_pos++;
                    }
                    
                    currChar = 0;
                    noAlt = 1;
                    while(altList[t][currChar] != '\0')
                    {
                        if(altList[t][currChar] == ',')
                            noAlt++;
                        currChar++;
                    };
                    
                                        
                    // newVTno = 0;
                    currChar = 0;
                    while (altList[t][currChar] != '\0')
                    {
                        a = 0;
                        
                        while (altList[t][currChar] != ',' && altList[t][currChar] != '\0')
                        {
                            alt[t][a] = altList[t][currChar];
                            a++;
                            currChar++;
                        }
                        alt[t][a] = '\0';
                        if (altList[t][currChar] == ',')
                            currChar++;
                        
                        ///*** just to check if all variants are OK
                        if (strcmp("PASS", filter[t]) != 0 && strcmp(".", filter[t]) != 0)
                        {
                            printf("*** WARNING ***\n");
                            printf("\tPosition: %d. Variant's FILTER value is neither 'PASS' nor '.': %s\n", pos, filter[t]);
                            printf("\tVariant will be processed anyway. If it should not, it should be removed from the VCF file\n");
                            
                        }
                        
                        if ((strstr(info[t], "VT=SNP") != NULL) || ( strstr(chrom[t], "Y") != NULL && strlen(refC[t]) == 1 && strlen(alt[t]) == 1))  //VT is a SNP or chrY(only SNPs)
                        {
                            if (strlen(refC[t]) != 1)
                            {
                                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                printf("\tPosition: %d. It is a SNP and REF length is not 1.\n", pos);
                                variantOK = false;
                                break;
                            }
                            if (strlen(alt[t]) != 1)
                            {
                                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                printf("\tPosition: %d. It is a SNP and ALT length is not 1\n", pos);
                                
                                
                                variantOK = false;
                                break;
                            }
                            
                            
                            while(1)
                            {
                                //int aaa = buf_max[t];
                                returnValue = snprintf(VLbuffer[t] + VLbufPos[t], buf_max[t] - VLbufPos[t], "%d SNP %s\n", pos, alt[t]);
                                if(returnValue > 0 && returnValue < buf_max[t] - VLbufPos[t])
                                {
                                    VLbufPos[t] +=  returnValue;
                                    break;
                                }
                                else
                                {
                                    VLbuffer[t] = (char *) realloc(VLbuffer[t], (buf_max[t]*2)* sizeof(char));
                                    buf_max[t] = buf_max[t]*2;
                                }
                            }
                        }
                        else if (strstr(info[t], "VT=INDEL") != NULL)  //VT is an INDEL
                        {
                            if ((255&alt[t][0]) != (255&refC[t][0]))
                            {
                                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                printf("\tPosition: %d. It is an INDEL and first characters of ALT and REF do not match.\n", pos);
                                variantOK = false;
                                break;
                            }
                            
                            if(strlen(refC[t])==1) //insertion
                            {
                                while(1)
                                {
                                    returnValue = snprintf(VLbuffer[t] + VLbufPos[t], buf_max[t] - VLbufPos[t], "%d INS %s\n", pos, alt[t]+1);
                                    if(returnValue > 0 && returnValue < buf_max[t] - VLbufPos[t])
                                    {
                                        VLbufPos[t] +=  returnValue;
                                        break;
                                    }
                                    else
                                    {
                                        VLbuffer[t] = (char *) realloc(VLbuffer[t], (buf_max[t]*2)* sizeof(char));
                                        buf_max[t] = buf_max[t]*2;
                                    }
                                }
                                
                                
                            }
                            else //if (strlen(alt[t])==1) //deletion
                            {
                                while(1)
                                {
                                    returnValue = snprintf(VLbuffer[t] + VLbufPos[t], buf_max[t] - VLbufPos[t],  "%d DEL %ld\n", pos, strlen(refC[t])-1);
                                    if(returnValue > 0 && returnValue < buf_max[t] - VLbufPos[t])
                                    {
                                        VLbufPos[t] +=  returnValue;
                                        break;
                                    }
                                    else
                                    {
                                        VLbuffer[t] = (char *) realloc(VLbuffer[t], (buf_max[t]*2)* sizeof(char));
                                        buf_max[t] = buf_max[t]*2;
                                    }
                                }
                                
                            }
                            
                        }
                        else if (strstr(info[t], "VT=SV") != NULL)  //VT is a SV
                        {
                            if (strstr(info[t], "SVTYPE=DEL") != NULL)
                            {
                                endPos[t][0] = '\0';
                                if (strstr(info[t], ";END=") != NULL)
                                {
                                    chr_ptr = strstr(info[t], ";END=");
                                    temp_int = 5;
                                    while (chr_ptr[temp_int] != ';' &&  chr_ptr[temp_int] != '\0')
                                    {
                                        endPos[t][temp_int-5] =  chr_ptr[temp_int];
                                        temp_int++;
                                    }
                                    endPos[t][temp_int-5]='\0';
                                }
                                else if (strncmp(info[t], "END=", 4) == 0)
                                {
                                    temp_int=4;
                                    while (info[t][temp_int] != ';' &&  info[t][temp_int] != '\0')
                                    {
                                        endPos[t][temp_int-4] =  info[t][temp_int];
                                        temp_int++;
                                    }
                                    endPos[j-4]='\0';
                                }
                                else
                                {
                                    printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                    printf("\tPosition: %d. SVTYPE=DEL and no END in info: %s\n", pos, info[t]);
                                    
                                    variantOK = false;
                                    break;
                                }
                                delLen = atoi(endPos[t])-pos;
                                
                                if (strncmp(alt[t], "<DEL>", 5) == 0) {
                                    isInsertion=false;
                                }
                                else if (strlen(alt[t]) == 1 && alt[t][0]==refC[t][0])
                                {
                                    isInsertion=false;
                                }
                                else if (strlen(alt[t]) > 1 && alt[t][0]==refC[t][0])
                                {
                                    isInsertion=true;
                                }
                                else {
                                    printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                    printf("\tPosition: %d. SVTYPE=DEL and for given REF, wrong ALT data: %s\n", pos, alt[t]);
                                    
                                    variantOK = false;
                                    break;
                                }
                                
                                if (isInsertion)
                                {
                                    
                                    while(1)
                                    {
                                        returnValue = snprintf(VLbuffer[t] + VLbufPos[t], buf_max[t] - VLbufPos[t],  "%d SV %d %s\n", pos, delLen, alt[t]+1);
                                        if(returnValue > 0 && returnValue < buf_max[t] - VLbufPos[t])
                                        {
                                            VLbufPos[t] +=  returnValue;
                                            break;
                                        }
                                        else
                                        {
                                            VLbuffer[t] = (char *) realloc(VLbuffer[t], (buf_max[t]*2)* sizeof(char));
                                            buf_max[t] = buf_max[t]*2;
                                        }
                                    }
                                    
                                }
                                else
                                {
                                    
                                    while(1)
                                    {
                                        returnValue = snprintf(VLbuffer[t] + VLbufPos[t], buf_max[t] - VLbufPos[t],  "%d SV %d -\n", pos, delLen);
                                        if(returnValue > 0 && returnValue < buf_max[t] - VLbufPos[t])
                                        {
                                            VLbufPos[t] +=  returnValue;
                                            break;
                                        }
                                        else
                                        {
                                            VLbuffer[t] = (char *) realloc(VLbuffer[t], (buf_max[t]*2)* sizeof(char));
                                            buf_max[t] = buf_max[t]*2;
                                        }
                                    }
                                    
                                    
                                }
                                
                                
                                
                            }
                            else
                            {
                                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                                printf("\tUnsupported variant VT=SV at position: %d  and not SVTYPE=DEL, INFO field:  %s\n",  pos, info[t]);
                                
                                variantOK = false;
                                break;
                            }
                            
                        }
                        else
                        {
                            printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                            printf("\tUnsupported variant at position: %d  (neither SNP, INDEL nor SV), INFO field: %s \n", pos, info[t]);
                            
                            
                            variantOK = false;
                            break;
                        }
                        
                        
                    }
                    
                    if(!variantOK)
                        continue;
                    
                    
                    // if there is a change in number of bitsCalculated that requires another unsigned char for storage, reallocate
                    if ((bitsCalculated[t]+noAlt) > char_max[t]*8)
                    {
                        for (int g = 0; g < noGen; g++)
                        {
                            
                            tempBits[t][g] = (unsigned char*) realloc(tempBits[t][g], (char_max[t] + 1)* sizeof(unsigned char));
                            tempBits[t][g][char_max[t]] = 0x0;
                        }
                        char_max[t] = char_max[t] + 1;
                        
                    }
                    
                    
                    for (int g = 0; g < noGen; g++)
                    {
                        
                        sscanf(line[currentLine]+ptr, "%s%n", genotype[t], &mv);
                        ptr = ptr + mv;
                        
                        
                        pp = strtok_r(genotype[t], ":", &strtokState2);
                        for(int m=0; m < gt_pos; m++)
                            pp = strtok_r( NULL, ":", &strtokState2);
                        
                        
                        
                        gt = atoi(pp);
                        
                        if (gt > noAlt)
                        {
                            printf("*** WARNING***\n");
                            printf("\tPosition: %d, GT data for sample no %d is wrong: %s, no variant will be placed in the consensus sequence here\n", currentPos, i+1, data);
                            gt = 0;
                        }
                        
                    
                        
                        for (a = 0; a < noAlt; a++)
                        {
                            if(gt == a+1)
                            {
                                shift = (8*sizeof(unsigned char)-1-(bitsCalculated[t]+a)%(8*sizeof(unsigned char)));
                                
                                tempBits[t][g][(bitsCalculated[t]+a)/(8*sizeof(unsigned char))] = tempBits[t][g][(bitsCalculated[t]+a)/(8*sizeof(unsigned char))]|((unsigned char)1<<shift);
                            }
                            
                        }
                        
                    }
                    bitsCalculated[t] = bitsCalculated[t] + noAlt;
                }
            }
        }
        
        
        
        
        // unsigned char toWrite;
        for (int t = 0; t < noThreads; t++)
        {
            //total += bitsCalculated[t];
            //            printf("%d", (bitsCalculated[t]));
            for (int g = 0; g < noGen; g++)
            {
                for (int b = 0; b <= (bitsCalculated[t]-1)/8; b++)
                {
                    
                    writeno = 8;
                    if(bitsCalculated[t]<8)
                        writeno = bitsCalculated[t];
                    if((b == (bitsCalculated[t]-1)/8 ) && bitsCalculated[t]%8 != 0)
                        writeno = bitsCalculated[t]%8;
                    
                    writeBits(bvName[g], buf[g], bufPos[g], tempBits[t][g][b], writeno);
                    
                }
            }
            
            l = 0;
            for (int v = 1; v <= bitsCalculated[t]; v++)
            {
                fprintf(out_vl, "%d ", total + v);
                while(VLbuffer[t][l] != '\n')
                {
                    putc(VLbuffer[t][l++], out_vl);
                    
                }
                putc(VLbuffer[t][l++], out_vl);
                
            }
            
            total += bitsCalculated[t];
            
        }
        
        
        if (chl != EOF)
            chl = getc(vcf);
        fseek( vcf, -1, SEEK_CUR);
    }
    
    
    // printf("total: %d\n", total);
    for (i=0; i < noGen; i++)
    {
        flushBuffer(bvName[i], buf[i], bufPos[i]);
        delete [] buf[i];
        
    }
    
    for (i = 0; i < NO_LINES_PER_THREAD * noThreads; i++)
        free(line[i]);
    free(line);
    
    for (i = 0; i < noThreads; i++)
    {
        for (j = 0; j < noGen; j++)
        {
            free(tempBits[i][j]);
        }
        
        free(tempBits[i]);
        
    }
    free(tempBits);
    
    
    
    fclose(vcf);
    fclose(out_vl);
    
    
    
    printf("Processed 100%% of the input vcf\n");
    return 0;
}