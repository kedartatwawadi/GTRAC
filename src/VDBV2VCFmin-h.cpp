#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <iostream>

#define BUFFER_SIZE 512000000


#define NO -1
#define SNP 0
#define INS 1
#define DEL 2
#define SV  3
#define OTHER  4


char * buffer;
int bufPos;
FILE * result;

struct variant
{
    unsigned short type;
    unsigned int position;
    char * alt;
    unsigned int delLen;
};


void putChar2Buf(char ch)
{
    if(bufPos  + 1 == BUFFER_SIZE)
    {
        fwrite(buffer, sizeof(char), bufPos, result);
        bufPos = 0;
    }
    
    buffer[bufPos++] = ch;
    
    
}

void putArr2Buf(char * arr)
{
    if(bufPos + strlen(arr) + 1 == BUFFER_SIZE)
    {
        fwrite(buffer, sizeof(char), bufPos, result);
        bufPos = 0;
        
    }
    
    for (int c = 0; c < strlen(arr); c++)
        buffer[bufPos++] = arr[c];
    
}


void putConsChr2Buf(const char * arr)
{
    if(bufPos + strlen(arr) + 1 == BUFFER_SIZE)
    {
        fwrite(buffer, sizeof(char), bufPos, result);
        bufPos = 0;
        
    }
    for (int c = 0; c < strlen(arr); c++)
        buffer[bufPos++] = arr[c];
    
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


int getNextSymbol(FILE *file) {
	int ch;
	while(true) {
		ch = getc(file);
		if (ch != '\n')
			break;
	}
	return ch;
	
}

//input arguments: reference, variant database, chrName, startPos, bitDataVector(s) [at least 1 (haploids), VCF will be made for them]
int main (int argc, char * const argv[]) {
    
    int startPos;
    char *chrom, ch;
    
    chrom = new char[strlen(argv[3])];
    chrom = argv[3];
    startPos = atoi(argv[4]);
    
    int inBitV = argc-5;
    if (inBitV < 1)
    {
        printf("Required arguments: reference, variant database, chrName, startPos, bitVectors [1+]\n");
        exit(9);
    }
    
    
	FILE * ref; //input file with reference seuence
	FILE * vl;	//input file with variants (light)
    
    printf("input reference: %s\n", argv[1]);
	ref = fopen(argv[1], "r");
	if (ref == NULL) {
		printf("Cannot open %s\n", argv[1]);
        printf("The error is - %s\n", strerror(errno));
		exit(8);
	}
    
	printf("input variant database: %s\n", argv[2]);
	vl = fopen(argv[2], "r");
	if (vl == NULL) {
		printf("Cannot open %s\n", argv[2]);
        printf("The error is - %s\n", strerror(errno));
		exit(8);
	}
    setvbuf(vl, NULL, _IOFBF, 1 << 22);
    
    
    char * resultFileName;
    resultFileName = new char[strlen(argv[2])+ 9];
    resultFileName[0]='\0';
    strncat(resultFileName, argv[2], strlen(argv[2]));
    strcat(resultFileName, ".min");
    strcat(resultFileName, "\0");
    result = fopen(resultFileName, "w");
    if (result == NULL) {
        printf("Cannot open %s", resultFileName);
        printf("The error is - %s\n", strerror(errno));
        exit(8);
    }
    std::cout << "result: " << resultFileName << "\n";
    
    
    int noVariants = 0;
    while (EOF != (ch=getc(vl)))
        if ('\n' == ch)
            ++noVariants;
    fseek(vl, 0, SEEK_SET);
    
    
	FILE * bitData;
    variant * vt;
    vt = (variant *) malloc (noVariants * sizeof(variant));
    char type[10];
    char temp[1000000];
    
    
    
    for(int i=0; i < noVariants; i++)
    {
        
        fscanf(vl, "%s %u %s ", (char*)&temp, &vt[i].position, type);
        
        if (strstr(type, "SNP") != NULL)  //VT is a SNP
        {
            vt[i].type = SNP;
            vt[i].alt = new char[1];
            fscanf(vl, "%s\n", vt[i].alt);
            vt[i].delLen = 0;
        }
        else if (strstr(type, "INS") != NULL)  //VT is an INS
        {
            vt[i].type = INS;
            fscanf(vl, "%s\n", temp);
            vt[i].alt = new char[strlen(temp)+1];
            memcpy(vt[i].alt , temp, strlen(temp));
            vt[i].alt[strlen(temp)]='\0';
            vt[i].delLen = 0;
            
        }
        else if (strstr(type, "DEL") != NULL)  //VT is an DEL
        {
            vt[i].type = DEL;
            fscanf(vl, "%u\n", &vt[i].delLen);
            vt[i].alt = new char[1];
            vt[i].alt[0] = '-';
        }
        else if (strstr(type, "SV") != NULL)  //VT is an SV
        {
            vt[i].type = SV;
            fscanf(vl, "%u %s\n", &vt[i].delLen, temp);
            vt[i].alt = new char[strlen(temp)+1];
            memcpy(vt[i].alt , temp, strlen(temp));
            vt[i].alt[strlen(temp)]='\0';
        }
    }
    
    
	int  i,  j =0, samePos=0;
  	
	//buffer = new char[BUFFER_SIZE];
	
    buffer = (char *) malloc(BUFFER_SIZE * sizeof(char));
    
    bufPos = 0;
    int returnValue;
    
	char  **inputBuffer;
    inputBuffer = new char*[inBitV];
    
    
	
	int pos,  currentPos = 0, allele;
    
	
	
    int  chRef;
    unsigned long int refSize, logTreshold;
    size_t tmpPt;
    
	
	
    
    
    fprintf(result, "##fileformat=VCFv4.1\n");
    fprintf(result, "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\tFORMAT");
    
    
    long bitVectorSize;
    //read bitvector first 
    printf("\rinput bitData: %s        (%d of %d) ", argv[5], 1, inBitV);
    bitData = fopen(argv[5], "rb");
    if (bitData == NULL) {
        printf("Cannot open %s\n", argv[5]);
        printf("The message is - %s\n", strerror(errno));
        exit(8);
    }
    
    fseek(bitData, 0, SEEK_END);
    bitVectorSize = ftell(bitData);
    fseek(bitData, 0, SEEK_SET);
    fclose(bitData);

    
    
    char *p = new char[100];
    for (i=0; i < inBitV; i++)
	{
        
        //read bitvector first
        printf("\rinput bitData: %s (%d of %d)", argv[5+i], i+1, inBitV);
        bitData = fopen(argv[5+i], "rb");
        if (bitData == NULL) {
            printf("Cannot open %s\n", argv[5+i]);
            printf("The error is - %s\n", strerror(errno));
            exit(8);
        }
        

        
        inputBuffer[i] = new char[bitVectorSize];
        fread(inputBuffer[i], 1, bitVectorSize, bitData);
        
        //write individual id
        
        p = strtok( argv[5+i], "." );
        
        
        if (strrchr(p,'/') != NULL)
            fprintf(result, "\t%s", strrchr(p,'/')+1);
        else
            fprintf(result, "\t%s", p);
        
        fclose(bitData);
        
        
    }
    fprintf(result, "\n");
    
    char *inputBuffer_t = new char[bitVectorSize * inBitV];
    for(int i = 0; i < bitVectorSize; ++i)
    {
        int offset = i * inBitV;
        for(int j = 0; j < inBitV; ++j)
        {
            inputBuffer_t[offset + j] = inputBuffer[j][i];
        }
    }

	
    
    
	i=0;
    
    chRef=getc(ref);
	while (chRef != '\n') {
        chRef=getc(ref);
	}
    
    
    
    
    tmpPt = ftell(ref);
    fseek( ref, 0, SEEK_END);
	refSize = ftell(ref);
    fseek( ref, tmpPt, SEEK_SET);
    logTreshold = startPos+ refSize/5;
    
    
    
    printf("\nProcessing...\n");
    
	currentPos = startPos;
    
    int currVT = 0, pos1, mask1 = 0, pos2, mask2 =0;
    
    pos = vt[currVT].position;
	
	while (currentPos < vt[currVT].position){
		chRef = getNextSymbol(ref);
		currentPos++;
	}
    
    bool *vtPresent;
	vtPresent = new bool[inBitV];
    int cntAlleles;
    
    char number[20];
    int n;
	while( currVT < noVariants)
	{
        if(currentPos > logTreshold)
        {
            printf("Processed %.f%% of reference sequence\n", (float)(logTreshold-startPos)/(float)(refSize)*100);
            logTreshold = logTreshold + refSize/5;
        }
        
        
        
		while (currentPos < vt[currVT].position) {
			
			chRef = getNextSymbol(ref);
			currentPos++;
		}
		
		if(!samePos)
			chRef = getNextSymbol(ref);
		
		
		if (vt[currVT].type == SNP)  //VT is a SNP
        {
            
            if(currVT+2 == noVariants|| vt[currVT+1].position > pos || vt[currVT+1].type != SNP )
                cntAlleles = 1;
            else
            {
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == SNP && vt[currVT+cntAlleles].position == pos)
                    cntAlleles++;
            }
            /*
            putArr2Buf(chrom);
            putChar2Buf('\t');
            num2char(vt[currVT].position,number);
            putArr2Buf(number);
            putConsChr2Buf("\t.\t");
            putChar2Buf(chRef);
            putChar2Buf('\t');*/
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%s\t%d\t.\t%c\t", chrom, pos, chRef);
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }
            
            //alt alleles here
            for(int i=0; i < cntAlleles-1; i++)
            {
                putArr2Buf(vt[currVT+i].alt);
                putChar2Buf(',');
                
            }
            //last allel (without ',')
            /*putArr2Buf(vt[currVT+cntAlleles-1].alt);
            putConsChr2Buf("\t.\tPASS\tVT=SNP\tGT");
            */
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%s\t.\tPASS\tVT=SNP\tGT", vt[currVT+cntAlleles-1].alt);
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }
            
           
            pos1 = currVT/8;
            mask1 = 1<<(7-currVT%8);
            for (j=0; j < inBitV; j++)
            {
                if(cntAlleles == 1)
                {
                    allele = (inputBuffer_t[pos1*inBitV + j] & mask1) != 0;
                }
                else
                {
                    allele = 0;
                    for(int i=0; i < cntAlleles; i++)
                    {
                        if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                        {
                            allele = i+1;
                            break;
                        }
                        
                    }
                }
                
                if (bufPos + 2 == BUFFER_SIZE)
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                }
                if(cntAlleles == 1)
                {
                    buffer[bufPos++] = '\t';
                    buffer[bufPos++] = '0' + (char) allele;
                }
                else
                {
                    buffer[bufPos++] = '\t';
                    if(allele < 9)
                        buffer[bufPos++] = (int)'0'+allele;
                    else
                        for(n = 0; n < num2char(allele, number); n++)
                            buffer[bufPos++] = number[n];
                }
                
            }
        }
        else if (vt[currVT].type == INS)  //VT is an INDEL
        {
            if(currVT+2 == noVariants || vt[currVT+1].position > pos || vt[currVT+1].type != INS)
                cntAlleles = 1;
            else
            {
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == INS && vt[currVT+cntAlleles].position == pos)
                    cntAlleles++;
                
            }
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%s\t%d\t.\t%c\t", chrom, pos, chRef);
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }
            
            
            for(int i=0; i < cntAlleles-1; i++)
            {
                while(1)
                {
                    returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%c%s,", chRef, vt[currVT+i].alt);
                    if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                    {
                        bufPos +=  returnValue;
                        break;
                    }
                    else
                    {
                        fwrite(buffer, sizeof(char), bufPos, result);
                        bufPos = 0;
                        
                    }
                }
                
            }
            //last allel (without ',')
            
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%c%s\t.\tPASS\tVT=INDEL\tGT", chRef,vt[currVT+cntAlleles-1].alt);
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                }
            }
            
            
            pos1 = currVT/8;
            mask1 = 1<<(7-currVT%8);
            for (j=0; j < inBitV; j++)
            {
                if(cntAlleles == 1)
                {
                    allele = (inputBuffer_t[pos1*inBitV + j] & mask1) != 0;
                }
                else
                {
                    allele = 0;
                    for(int i=0; i < cntAlleles; i++)
                    {
                        if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                        {
                            allele = i+1;
                            break;
                        }
                        
                    }
                }
                
                if (bufPos + 2 == BUFFER_SIZE)
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                }
                if(cntAlleles == 1)
                {
                    buffer[bufPos++] = '\t';
                    buffer[bufPos++] = '0' + (char) allele;
                }
                else
                {
                    buffer[bufPos++] = '\t';
                    if(allele < 9)
                        buffer[bufPos++] = (int)'0'+allele;
                    else
                        for(n = 0; n < num2char(allele, number); n++)
                            buffer[bufPos++] = number[n];
                }
                
            }
        }
        else if (vt[currVT].type == DEL) //deletion
        {
            
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos,"%s\t%d\t.\t%c", chrom, pos, chRef);
                
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }
            
            
            tmpPt = ftell(ref);
            for (j=0; j < vt[currVT].delLen; j++)
            {
                putChar2Buf(getNextSymbol(ref));
                
            }
            fseek( ref, tmpPt, SEEK_SET);
            
            putChar2Buf('\t');
            putChar2Buf(chRef);
            putConsChr2Buf("\t.\tPASS\tVT=INDEL\tGT");
            
            cntAlleles=1;
            
            
            pos1 = currVT/8;
            mask1 = 1<<(7-currVT%8);
            for (j=0; j < inBitV; j++)
            {
                if(cntAlleles == 1)
                {
                    allele = (inputBuffer_t[pos1*inBitV + j] & mask1) != 0;
                }
                else
                {
                    allele = 0;
                    for(int i=0; i < cntAlleles; i++)
                    {
                        if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                        {
                            allele = i+1;
                            break;
                        }
                        
                    }
                }
                
                if (bufPos + 2 == BUFFER_SIZE)
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                }
                if(cntAlleles == 1)
                {
                    buffer[bufPos++] = '\t';
                    buffer[bufPos++] = '0' + (char) allele;
                }
                else
                {
                    buffer[bufPos++] = '\t';
                    if(allele < 9)
                        buffer[bufPos++] = (int)'0'+allele;
                    else
                        for(n = 0; n < num2char(allele, number); n++)
                            buffer[bufPos++] = number[n];
                }
                
            }
            
        }
        else if (vt[currVT].type == SV)  //VT is a SV
        {
            
            while(1)
            {
                returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos,"%s\t%d\t.\t%c", chrom, pos, chRef);
                
                if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                {
                    bufPos +=  returnValue;
                    break;
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }
            
            
            tmpPt = ftell(ref);
            for (j=0; j < vt[currVT].delLen; j++)
            {
                putChar2Buf(getNextSymbol(ref));
            }
            fseek( ref, tmpPt, SEEK_SET);
            
            
            //sth different than a SV at the same pos - do nothing
            if(currVT+2 == noVariants || vt[currVT+1].position > pos || vt[currVT+1].type != INS)
                cntAlleles = 1;
            else
            {
                
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == SV && vt[currVT+cntAlleles].position == pos)
                    cntAlleles++;
                
                
            }
            
            putChar2Buf('\t');
            /*
            while(1)
            {
                
                if (bufPos+1 < BUFFER_SIZE)
                {
                    buffer[bufPos++] = '\t';
                    break;
                    
                }
                else
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                    
                }
            }*/
            //alt alleles here
            for( i=0; i < cntAlleles-1; i++)
            {
                while(1)
                {
                    if( vt[currVT+i].alt[0] != '-')
                    {
                        returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos,"%c%s,", chRef, vt[currVT+i].alt);
                        
                        if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                        {
                            bufPos +=  returnValue;
                            break;
                        }
                        else
                        {
                            fwrite(buffer, sizeof(char), bufPos, result);
                            bufPos = 0;
                        }
                    }
                    else
                    {
                        returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos,"%c,", chRef);
                        
                        if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                        {
                            bufPos +=  returnValue;
                            break;
                        }
                        else
                        {
                            fwrite(buffer, sizeof(char), bufPos, result);
                            bufPos = 0;
                        }
                        
                    }
                    
                }
                
                
            }
            //last allel (without ',')
            
            while(1)
            {
                if( vt[currVT+i].alt[0] != '-')
                {
                    returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%c%s\t.\tPASS\tVT=SV;SVTYPE=DEL;END=%d\tGT",chRef, vt[currVT+cntAlleles-1].alt, currentPos+vt[currVT+cntAlleles-1].delLen);
                    
                    if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                    {
                        bufPos +=  returnValue;
                        break;
                    }
                    else
                    {
                        fwrite(buffer, sizeof(char), bufPos, result);
                        bufPos = 0;
                        
                    }
                    
                }
                else
                {
                    returnValue = snprintf(buffer + bufPos, BUFFER_SIZE - bufPos, "%c\t.\tPASS\tVT=SV;SVTYPE=DEL;END=%d\tGT", chRef, currentPos+vt[currVT+cntAlleles-1].delLen);
                    
                    if(returnValue > 0 && returnValue < BUFFER_SIZE - bufPos)
                    {
                        bufPos +=  returnValue;
                        break;
                    }
                    else
                    {
                        fwrite(buffer, sizeof(char), bufPos, result);
                        bufPos = 0;
                        
                    }
                    
                }
                
            }
            
            
            
            pos1 = currVT/8;
            mask1 = 1<<(7-currVT%8);
            for (j=0; j < inBitV; j++)
            {
                if(cntAlleles == 1)
                {
                    allele = (inputBuffer_t[pos1*inBitV + j] & mask1) != 0;
                }
                else
                {
                    allele = 0;
                    for(int i=0; i < cntAlleles; i++)
                    {
                        if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                        {
                            allele = i+1;
                            break;
                        }
                        
                    }
                }
                
                if (bufPos + 2 == BUFFER_SIZE)
                {
                    fwrite(buffer, sizeof(char), bufPos, result);
                    bufPos = 0;
                }
                if(cntAlleles == 1)
                {
                    buffer[bufPos++] = '\t';
                    buffer[bufPos++] = '0' + (char) allele;
                }
                else
                {
                    buffer[bufPos++] = '\t';
                    if(allele < 9)
                        buffer[bufPos++] = (int)'0'+allele;
                    else
                        for(n = 0; n < num2char(allele, number); n++)
                            buffer[bufPos++] = number[n];
                }
                
            }
            
            
            
        }
        else{ printf("Unknown variant type nr: %d\n", currVT);}
        
        putChar2Buf('\n');
        /*
        while(1)
        {
            
            if (bufPos+1 < BUFFER_SIZE)
            {
                buffer[bufPos++] = '\n';
                break;
                
            }
            else
            {
                fwrite(buffer, sizeof(char), bufPos, result);
                bufPos = 0;
            }
        }*/
		currVT += cntAlleles;
        pos = vt[currVT].position;
        
        
        samePos = 0;
		if(pos == currentPos)
			samePos = 1;
		else
			currentPos++;
        
        
    }
	
    fwrite(buffer, 1, bufPos, result);
    
	fclose(result);
    
	fclose(ref);
	fclose(vl);
 
    
    printf("Processed 100%% of reference sequence\n");
    return 0;
}

