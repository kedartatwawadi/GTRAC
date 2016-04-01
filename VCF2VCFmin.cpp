

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <iostream>

#define BUFFER_SIZE 5120000

FILE * res;
char * buffer;
int bufPos;

void clearBuffer(unsigned char * buf, int &bufPos, int size)
{
    for (int i=0; i < size; i++)
    {
        buf[i] = 0;
    }
    bufPos = 0;
}

void flushBuffer(FILE * file, unsigned char * buf, int &bufPos)
{
    int last = 0;
    if (bufPos%8 > 0)
        last = 1;
    fwrite(buf, 1, bufPos/8 + last, file);
}


char getNextSymbol(FILE *file) {
	char ch;
	while(true) {
		ch = getc(file);
		if (ch != '\n')
			break;
	}
	return ch;
	
}

void putChar2Buf(char ch)
{
    while(1)
    {
        if(bufPos  + 1 < BUFFER_SIZE)
        {
            buffer[bufPos++] = ch;
            break;
        }
        else
        {
            fwrite(buffer, sizeof(char), bufPos, res);
            bufPos = 0;
            
        }
    }
}

void putArr2Buf(char * arr)
{
    while(1)
    {
        if(bufPos + strlen(arr) + 1 < BUFFER_SIZE)
        {
            for (int c = 0; c < strlen(arr); c++)
                buffer[bufPos++] = arr[c];
            break;
        }
        else
        {
            fwrite(buffer, sizeof(char), bufPos, res);
            bufPos = 0;
            
        }
    }
}

void putConsChr2Buf(const char * arr)
{
    while(1)
    {
        if(bufPos + strlen(arr) + 1 < BUFFER_SIZE)
        {
            for (int c = 0; c < strlen(arr); c++)
                buffer[bufPos++] = arr[c];
            break;
        }
        else
        {
            fwrite(buffer, sizeof(char), bufPos, res);
            bufPos = 0;
            
        }
    }
}


int num2char(int num, char * out)
{
    char temp[21];
    int no_chars = 0;
    if(num == 0)
    {
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }
    else
        while (num)
        {
            temp[no_chars++] = (char)(((int)'0')+(num%10));
            num = num/10;
        }
    temp[no_chars] = '\0';
    
    for(int i = 0; i < no_chars; i++)
        out[i] = temp[no_chars - i - 1];
    out[no_chars] = '\0';
    return no_chars;
}



//***************************************************//

//input arg:  <vcf> <ref> [optional: startPosOfRef]
int main (int argc, char * const argv[]) {
    
    FILE * vcf;	//input file with variants
    FILE * ref; //input ref
	
	int pos,  currentPos = 0;
    
    printf("input vcf: %s\n", argv[1]);
    vcf = fopen(argv[1], "r");
    if (vcf == NULL) {
        printf("Cannot open %s\n", argv[1]);
        exit(8);
    }
    printf("input reference: %s\n", argv[2]);
	ref = fopen(argv[2], "r");
	if (ref == NULL) {
		printf("Cannot open %s\n", argv[2]);
		exit(8);
	}
    
    if(argc >= 4)
        currentPos = atoi(argv[3])-1;
    
    
    
    char * resultFileName;
    resultFileName = (char*) malloc ((strlen(argv[1])+6) * sizeof(char));
    
    resultFileName[0]='\0';
    strcat(resultFileName, argv[1]);
    
    strcat(resultFileName, ".min");
    printf("res: %s\n", resultFileName);
    
    res = fopen(resultFileName, "w");
    if (res == NULL) {
        printf("Cannot open %s\n", resultFileName);
        exit(8);
    }
    
	
    buffer = (char *) malloc(BUFFER_SIZE * sizeof(char));
    bufPos = 0;
    
    
	int  i,  j =0, gt_pos;
	char chrom[15];
    
    unsigned long int vcfSize, logTreshold;
    size_t tmpPt;
    
	
    
	char   *helpFileName, *ptr;
    helpFileName = new char[30];
    
	
	int delLen, endPosition, currChar, a, noAlt;
	
	char  name[1000], refC[1000000], alt[1000000], altList[1000000], filter[20], info[1000], format[100], data[200], qual[20], endPos[20];
    char genome[20], chRef, out[21];
    
    i = 1;j=0;
    char *p = new char[1000];
    
	
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
    //count no samples
    tmpPt = ftell(vcf);
    int noGen = 0;
    while (getc(vcf) != '\n')
    {
        fscanf(vcf, "%s", data);
        noGen ++;
    }
    fseek( vcf, tmpPt, SEEK_SET);
    
    putConsChr2Buf("##fileformat=VCFv4.1\n#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT");
    
    for (i=0; i < noGen; i++)
    {
        fscanf(vcf, "%s", genome);
        putChar2Buf('\t');
        putArr2Buf(genome);
        
    }
    
    putChar2Buf('\n');
    
    
    printf("Processing...\n");
    
    fscanf(vcf, "%s %d %s %s %s %s %s %s %s", chrom, &pos, name, refC, altList, qual, filter, info, format);
    
    p = strtok(format, ":");
    gt_pos = 0;
    while (strcmp(p,"GT") != 0) {
        p = strtok(NULL, ":");
        gt_pos++;
    }
    
    
    chRef = getc(ref);
	while (chRef != '\n') {
        chRef = getc(ref);
	}
    
    while (currentPos < pos){
		chRef = getNextSymbol(ref);
		currentPos++;
	}
    
    bool variantOK;
    while (!feof(vcf))
    {
        variantOK = true;
        if(ftell(vcf) > logTreshold)
        {
            printf("Processed %.f%% of the input vcf\n", (float)(logTreshold)/(float)(vcfSize)*100);
            logTreshold = logTreshold + vcfSize/5;
        }
        
        while (currentPos < pos) {
			
			chRef = getNextSymbol(ref);
			currentPos++;
		}
        
        
        if (strcmp("PASS", filter) != 0 && strcmp(".", filter) != 0)
        {
            printf("*** WARNING ***\n");
            printf("\tPosition: %d. Variant's FILTER value is neither 'PASS' nor '.': %s\n", pos, filter);
            printf("\tVariant will be processed anyway. If it should not, it should be removed from the VCF file\n");
            
        }
        
        if ((strstr(info, "VT=SNP") != NULL) || (strncmp(chrom, "Y", 1) == 0))  //VT is a SNP or chrY(only SNPs)
        {
            if (strlen(refC) != 1)
            {
                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                printf("\tPosition: %d. It is a SNP and REF length is not 1\n", pos);
                variantOK = false;
            }
            
            num2char(pos, out);
            putArr2Buf(chrom);
            putChar2Buf('\t');
            putArr2Buf(out);
            putConsChr2Buf("\t.\t");
            putArr2Buf(refC);
            putChar2Buf('\t');
            
            currChar = 0;
            noAlt = 0;
            while (altList[currChar] != '\0')
            {
                a = 0;
                while (altList[currChar] != ',' && altList[currChar] != '\0')
                {
                    alt[a] = altList[currChar];
                    a++;
                    currChar++;
                }
                alt[a] = '\0';
                noAlt++;
                
                if (altList[currChar] == ',')
                    currChar++;
                if(noAlt>1)
                    putChar2Buf(',');
                
                if (strlen(alt) != 1)
                {
                    printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                    printf("\tPosition: %d. It is a SNP and ALT length is not 1.\n", pos);
                    variantOK = false;
                    
                    
                }
                
                putArr2Buf(alt);
                
            }
            
            putConsChr2Buf("\t.\tPASS\tVT=SNP\tGT");
            
        }
        else if (strstr(info, "VT=INDEL") != NULL)  //VT is an INDEL
        {
            
            num2char(pos, out);
            putArr2Buf(chrom);
            putChar2Buf('\t');
            putArr2Buf(out);
            putConsChr2Buf("\t.\t");
            putArr2Buf(refC);
            putChar2Buf('\t');
            putArr2Buf(altList);
            putConsChr2Buf("\t.\tPASS\tVT=INDEL\tGT");
            
        }
        else if (strstr(info, "VT=SV") != NULL)  //VT is a SV
        {
            if (strstr(info, "SVTYPE=DEL") != NULL)
            {
                endPos[0] = '\0';
                if (strstr(info, ";END=") != NULL)
                {
                    ptr=strstr(info, ";END=");
                    j=5;
                    while (ptr[j] != ';' &&  ptr[j] != '\0')
                    {
                        endPos[j-5] =  ptr[j];
                        j++;
                    }
                    endPos[j-5]='\0';
                    
                }
                else if (strncmp(info, "END=", 4) == 0)
                {
                    j=4;
                    while (info[j] != ';' &&  info[j] != '\0')
                    {
                        endPos[j-4] =  info[j];
                        //endPos[j]=
                        j++;
                    }
                    endPos[j-4]='\0';
                }
                else
                {
                    printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                    printf("\tPosition: %d. SVTYPE=DEL and no END in info: %s\n", pos, info);
                    
                    variantOK = false;
                    
                    
                }
                
                endPosition  = atoi(endPos);
                delLen = atoi(endPos)-pos;
                
                num2char(pos, out);
                putArr2Buf(chrom);
                putChar2Buf('\t');
                putArr2Buf(out);
                putConsChr2Buf("\t.\t");
                putChar2Buf(chRef);
                
                tmpPt = ftell(ref);
                for (j=0; j < delLen; j++)
                {
                    putChar2Buf(getNextSymbol(ref));
                }
                fseek( ref, tmpPt, SEEK_SET);
                putChar2Buf('\t');
                
                
                currChar = 0;
                noAlt = 0;
                while (altList[currChar] != '\0')
                {
                    a = 0;
                    while (altList[currChar] != ',' && altList[currChar] != '\0')
                    {
                        alt[a] = altList[currChar];
                        a++;
                        currChar++;
                    }
                    alt[a] = '\0';
                    noAlt++;
                    
                    if (altList[currChar] == ',')
                        currChar++;
                    if(noAlt>1)
                        putChar2Buf(',');
                    
                    
                    if(strcmp(alt,"<DEL>")==0)
                        putChar2Buf(refC[0]);
                    
                    else
                        putArr2Buf(alt);
                }
                
                num2char(endPosition, out);
                putConsChr2Buf("\t.\tPASS\tVT=SV;SVTYPE=DEL;END=");
                putArr2Buf(out);
                putConsChr2Buf("\tGT");
                
                
            }
            else
            {
                printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
                printf("\tUnsupported variant VT=SV  and not SVTYPE=DEL: %s %d %s %s %s %s %s %s %s\n", chrom, pos, name, refC, alt, qual, filter, info, format);
                
                variantOK = false;
            }
            
        }
        else
        {
            printf("*** UNSUPPORTED CASE, variant will be omitted. ***\n");
            printf("\tUnsupported variant VT=SV at position: %d  and not SVTYPE=DEL, INFO field:  %s\n",  pos, info);
            
            variantOK = false;
        }
        
        
        
        if (variantOK)
        {
            for (i=0; i < noGen; i++)
            {
                fscanf(vcf, "%s", data);
                p = strtok( data, ":" );
                for(int m=0; m < gt_pos; m++)
                    p = strtok( NULL, ":" );
                if (strcmp(p,".") != 0)
                {
                    putChar2Buf('\t');
                    putArr2Buf(p);
                    
                }
                else
                {
                    putChar2Buf('\t');
                    putChar2Buf('0');
                }
            }
            putChar2Buf('\n');
            
            fscanf(vcf, "%s %d %s %s %s %s %s %s %s", chrom, &pos, name, refC, altList, qual, filter, info, format);
            p = strtok(format, ":");
            gt_pos = 0;
            while (strcmp(p,"GT") != 0) {
                p = strtok(NULL, ":");
                gt_pos++;
            }
        }
        else
        {
            while (getc(vcf) != '\n')
                ;
            fscanf(vcf, "%s %d %s %s %s %s %s %s %s", chrom, &pos, name, refC, altList, qual, filter, info, format);
            p = strtok(format, ":");
            gt_pos = 0;
            while (strcmp(p,"GT") != 0) {
                p = strtok(NULL, ":");
                gt_pos++;
            }
        }
    }
    
    
    fwrite(buffer, 1, bufPos, res);
    
    delete buffer;
    fclose(vcf);
    fclose(ref);
    
    printf("Processed 100%% of the input vcf\n");
    
    return 0;
}
