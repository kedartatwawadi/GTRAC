#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <errno.h>

#define LINE_WIDTH 70
#define BUFFER_SIZE 512000


#define NO -1
#define SNP 0
#define INS 1
#define DEL 2
#define SV  3
#define OTHER  4


char getNextSymbol(FILE *file) {
	char ch;
	while(true) {
		ch = getc(file);
		if (ch != '\n')
			break;
	}
	return ch;
	
}


void writeChar(char ch, char * buf, int &bufPos, int &lineCnt, char * filename)
{
    FILE *file;
	if(bufPos == BUFFER_SIZE)
	{
		file = fopen(filename, "a");
        fwrite(buf, sizeof(char), BUFFER_SIZE, file);
        fclose(file);
		bufPos = 0;
	}
	buf[bufPos]=ch;
	bufPos++;
    
	if(bufPos == BUFFER_SIZE)
	{
		file = fopen(filename, "a");
        fwrite(buf, sizeof(char), BUFFER_SIZE, file);
        fclose(file);

		bufPos = 0;
	}
	lineCnt++;
	if (lineCnt == LINE_WIDTH) {
		lineCnt=0;
		buf[bufPos] = '\n';
		bufPos++;
	}
}

void writeChar(int ch, char * buf, int &bufPos, int &lineCnt, char * filename)
{
    FILE *file;
	if(bufPos == BUFFER_SIZE)
	{
		file = fopen(filename, "a");
        fwrite(buf, sizeof(char), BUFFER_SIZE, file);
        fclose(file);

		bufPos = 0;
	}
	
	buf[bufPos++]=(255&ch);
	if(bufPos == BUFFER_SIZE)
	{
		file = fopen(filename, "a");
        fwrite(buf, sizeof(char), BUFFER_SIZE, file);
        fclose(file);

		bufPos = 0;
	}
	lineCnt++;
	if (lineCnt == LINE_WIDTH) {
		lineCnt=0;
		buf[bufPos] = '\n';
		bufPos++;
	}
}

struct variant
{
    unsigned short type;
    unsigned int position;
    char * alt;
    unsigned int delLen;
    
    bool isInsertion;
    bool doubleInsertion;
};

//input arguments: reference, variant-databse, startPos, bitDataVector(s) [at least 1]
int main (int argc, char * const argv[]) {
    
    int startPos;
    
    startPos = atoi(argv[3]);
    
    int inBitV = argc-4;
    
    if (inBitV < 1)
    {
        printf("Required arguments: reference, variant database, startPos, bitVectors [1+]\n");
        exit(9);
    }
    
	FILE * vl;	//input file with variants
	FILE * ref; //input file with reference seuence
	FILE * bitData;
	 char **resName;
    FILE * file;
    resName = new char *[inBitV];
    
	
	int  i, linePos[inBitV], j =0, samePos=0;
	int deleteSeq[inBitV];
    unsigned long insertSeq[inBitV];
    unsigned long prevIns1;
	char * buffer;
    char temp[1000000];
    
    char type[10];
    char ch;
	buffer = new char[BUFFER_SIZE];
	
	char ** buf, **inputBuffer;
	buf = new char*[inBitV];
    inputBuffer = new char*[inBitV];
    
    
	for (i=0; i < inBitV; i++)
	{
        
		buf[i] = new char[BUFFER_SIZE];
	}
	
	int bufferPos = 0 , bufPos[inBitV];
    
	int  currentPos = 0, allele;
    //  unsigned int noVar=0;
	
	bool doubleInsertion=false;
	
    
	
	int  chRef;
    short int prevVariant = NO;
	long int size;
	
    unsigned long int refSize, logTreshold;
    size_t tmpPt;
	
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
    
    int noVariants = 0;
    while (EOF != (ch=getc(vl)))
        if ('\n' == ch)
            ++noVariants;
    fseek(vl, 0, SEEK_SET);
    
    variant * vt;
    vt = (variant *) malloc (noVariants * sizeof(variant));
    
    for(i=0; i < noVariants; i++)
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
    
    
    
    
    for (i=0; i < inBitV; i++)
	{
        
        printf("\rinput bitData: %s (%d of %d) ", argv[4+i], i+1, inBitV);
        bitData = fopen(argv[4+i], "rb");
        if (bitData == NULL) {
            printf("Cannot open %s\n", argv[4+i]);
            printf("The error is - %s\n", strerror(errno));
            exit(8);
        }
        
        fseek(bitData, 0, SEEK_END);
        size = ftell(bitData);
        fseek(bitData, 0, SEEK_SET);
        
        inputBuffer[i] = new char[size];
        fread(inputBuffer[i], 1, size, bitData);
        fclose(bitData);
    }
	
	
	i=0;
    char header[1000];
    chRef = getc(ref);
	while (chRef != '\n') {
		header[i++]=chRef;
        chRef = getc(ref);
	}
    header[i++]='\n';
    header[i]='\0';
    
	
	for (i=0; i < inBitV; i++)
	{
        resName[i] = new char[strlen(argv[4+i])+ 10];
		linePos[i]=0;
		deleteSeq[i]=0;
        insertSeq[i]=0;
		bufPos[i]=0;
        
		resName[i][0]='\0';
		strncat(resName[i], argv[4+i], strlen(argv[4+i]));
		strcat(resName[i], ".fa");
		strcat(resName[i], "\0");
	/*	resultSeq[i] = fopen(resName[i], "w");
		if (resultSeq[i] == NULL) {
			printf("Cannot open %s", resultFileName);
			exit(8);
		}
        delete resultFileName;
*/		
	}
	
    
    for (i=0; i < inBitV; i++)
	{
        file = fopen(resName[i], "w");
        if (file == NULL) {
            printf("Cannot open %s\n", resName[i]);
            printf("The error is - %s\n", strerror(errno));
            exit(8);
        }
        fprintf(file, "%s", header);
        fclose(file);
	}
    
    
    tmpPt = ftell(ref);
    fseek( ref, 0, SEEK_END);
	refSize = ftell(ref);
    fseek( ref, tmpPt, SEEK_SET);
    logTreshold = startPos+ refSize/5;
    
    
    
    printf("\nProcessing...\n");
    
	currentPos = startPos;
    
    int currVT = 0;
    
	
	while (currentPos < vt[currVT].position){
		
		chRef = getNextSymbol(ref);
		
		if(bufferPos == BUFFER_SIZE)
		{
			for (i=0; i < inBitV; i++)
			{
                file = fopen(resName[i], "a");
                fwrite(buffer, 1, BUFFER_SIZE, file);
                fclose(file);
			}
			bufferPos = 0;
		}
		
		buffer[bufferPos]=(255&chRef);
        bufferPos++;
        if(bufferPos == BUFFER_SIZE)
        {
            for (i=0; i < inBitV; i++)
            {
                file = fopen(resName[i], "a");
                fwrite(buffer, 1, BUFFER_SIZE, file);
                fclose(file);
            }
            bufferPos = 0;
        }
        j++;
        if (j == LINE_WIDTH) {
            j=0;
            buffer[bufferPos]= '\n';
            bufferPos++;
        }
		
		
		
		currentPos++;
	}
	for (i=0; i < inBitV; i++)
	{
		file = fopen(resName[i], "a");
        fwrite(buffer, 1, bufferPos, file);
        fclose(file);
    }
	for (i=0; i < inBitV; i++)
	{
		linePos[i] = j;
	}
    
    bool *vtPresent;
	vtPresent = new bool[inBitV];
    int cntAlleles;
	
    
    
    while( currVT < noVariants)
	{
        if(currentPos > logTreshold)
        {
            printf("Processed %.f%% of reference sequence\n", (float)(logTreshold-startPos)/(float)(refSize)*100);
            logTreshold = logTreshold + refSize/5;
        }
        
		while (currentPos < vt[currVT].position) {
			
			chRef = getNextSymbol(ref);
			
            for (i=0; i < inBitV; i++)
			{
				if(deleteSeq[i] > 0)
				{
					deleteSeq[i]--;
				}
				else
				{
					writeChar(chRef, buf[i], bufPos[i], linePos[i], resName[i]);
				}
            }
			currentPos++;
		}
		
        if(!samePos)
			chRef = getNextSymbol(ref);
		
		
		if (vt[currVT].type == SNP)  //VT is a SNP
        {
            
            if(currVT+2 == noVariants|| vt[currVT+1].position > vt[currVT].position || vt[currVT+1].type != SNP )
                cntAlleles = 1;
            else
            {
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == SNP && vt[currVT+cntAlleles].position == vt[currVT].position)
                    cntAlleles++;
            }
            
            
            for (j=0; j < inBitV; j++)
            {
                allele=0;
                for(i=0; i < cntAlleles; i++)
                {
                    if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                    {
                        allele = i+1;
                        break;
                    }
                    
                }
                
                if(allele > 0) //some variant present
                {
                    if(deleteSeq[j] > 0)
                    {
                        deleteSeq[j]--;
                    }
                    else
                    {
                        writeChar(vt[currVT + allele - 1].alt[0], buf[j], bufPos[j], linePos[j], resName[j]);
                    }
                }
                else
                {
                    if(deleteSeq[j] > 0)
                    {
                        deleteSeq[j]--;
                    }
                    else
                    {
                        writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                    }
                }
            }
            prevVariant = SNP;
            
            
        }
        else if (vt[currVT].type == INS)  //VT is an INDEL
        {
            if(currVT+2 == noVariants || vt[currVT+1].position > vt[currVT].position || vt[currVT+1].type != INS)
                cntAlleles = 1;
            else
            {
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == INS && vt[currVT+cntAlleles].position == vt[currVT].position)
                    cntAlleles++;
                
            }
            
            
            if(samePos && prevVariant == INS)
            {
                doubleInsertion=true;
            }
            else
                doubleInsertion=false;
            
            
            prevVariant = INS;
            
            
            
            for (j=0; j < inBitV; j++)
            {
                allele=0;
                for(i=0; i < cntAlleles; i++)
                {
                    if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                    {
                        allele = i+1;
                        break;
                    }
                    
                }
                
                if(doubleInsertion)
                {
                    prevIns1 = insertSeq[j];
                }
                else
                {
                    prevIns1 = 0;
                }
                
                if(allele > 0) //some variant present
                {
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                    }
                    
                    for (i = 0; i < strlen(vt[currVT + allele - 1].alt); i++)
                    {
                        
                        if(doubleInsertion && insertSeq[j] > 0)
                        {
                            insertSeq[j]--;
                        }
                        else
                        {
                            writeChar(vt[currVT + allele - 1].alt[i], buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                        
                    }
                    insertSeq[j]=(strlen(vt[currVT + allele - 1].alt)) > prevIns1 ? (strlen(vt[currVT + allele].alt)) : prevIns1;
                }
                else
                {
                    if(!doubleInsertion)
                    {
                        insertSeq[j]=0;
                        
                    }
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                    }
                }
                
            }
            
            
        }
        else if (vt[currVT].type == DEL) //deletion
        {
            prevVariant = DEL;
            
            
            cntAlleles = 1;
            
            for (j=0; j < inBitV; j++)
            {
                allele=0;
                for(i=0; i < cntAlleles; i++)
                {
                    if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                    {
                        allele = i+1;
                        break;
                    }
                    
                }
                
                if(allele>0)
                {
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                    }
                    deleteSeq[j] = (deleteSeq[j]>(vt[currVT + allele - 1].delLen)) ? deleteSeq[j] :(vt[currVT + allele - 1].delLen) ;
                }
                else
                {
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                        
                    }
                }
            }
            
        }
        else if (vt[currVT].type == SV)  //VT is a SV
        {
            
            
            if(vt[currVT].alt[0] == '-')
            {
                vt[currVT].isInsertion=false;
                //prevVariant = DEL;
            }
            else
            {
                if(samePos && prevVariant == INS)
                {
                    vt[currVT].doubleInsertion = true;
                }
                else
                {
                    vt[currVT].doubleInsertion = false;
                }
                vt[currVT].isInsertion=true;
                //prevVariant = INS;
            }
            
            
            //sth different than a SV at the same pos - do nothing
            if(currVT+2 == noVariants || vt[currVT+1].position > vt[currVT].position || vt[currVT+1].type != SV)
                cntAlleles = 1;
            else
            {
                
                cntAlleles = 1;
                while (vt[currVT+cntAlleles].type == SV && vt[currVT+cntAlleles].position == vt[currVT].position)
                {
                    if(vt[currVT + cntAlleles].alt[0] == '-')
                    {
                        vt[currVT+cntAlleles].isInsertion=false;
                        // prevVariant = DEL;
                    }
                    else
                    {
                        if(samePos && prevVariant == INS)
                        {
                            vt[currVT+cntAlleles].doubleInsertion = true;
                        }
                        else
                        {
                            vt[currVT+cntAlleles].doubleInsertion = false;
                        }
                        vt[currVT+cntAlleles].isInsertion = true;
                        // prevVariant = INS;
                    }
                    cntAlleles++;
                    
                }
                
            }
            
            
            for (j=0; j < inBitV; j++)
            {
                allele = 0;
                for(i=0; i < cntAlleles; i++)
                {
                    if( (inputBuffer[j][(currVT+i)/8] & 1<<(7-(currVT+i)%8)) > 0)
                    {
                        allele = i+1;
                        break;
                    }
                    
                }
                if(allele > 0)
                {
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                    }
                    
                    deleteSeq[j] = (deleteSeq[j] > vt[currVT + allele - 1].delLen) ? deleteSeq[j] : vt[currVT + allele - 1].delLen ;
                    
                    if(vt[currVT + allele - 1].isInsertion)
                    {
                        
                        if(vt[currVT + allele - 1].doubleInsertion)
                        {
                            prevIns1 = insertSeq[j];
                        }
                        else
                        {
                            prevIns1 = 0;
                        }
                        for (i = 0; i < strlen(vt[currVT + allele - 1].alt); i++)
                        {
                            
                            if(vt[currVT + allele - 1].doubleInsertion && insertSeq[j] > 0)
                            {
                                insertSeq[j]--;
                            }
                            else
                            {
                                writeChar(vt[currVT + allele - 1].alt[i], buf[j], bufPos[j], linePos[j], resName[j]);
                            }
                            
                        }
                        insertSeq[j]=(strlen(vt[currVT + allele - 1].alt)) > prevIns1 ? (strlen(vt[currVT + allele - 1].alt)) : prevIns1;
                    }
                }
                else
                {
                    if(!samePos)
                    {
                        if(deleteSeq[j] > 0)
                        {
                            deleteSeq[j]--;
                        }
                        else
                        {
                            writeChar(chRef, buf[j], bufPos[j], linePos[j], resName[j]);
                        }
                    }
                    
                    
                    for (int a = 0; a < cntAlleles; a++)
                    {
                        if(vt[currVT + a].isInsertion && !vt[currVT + a].doubleInsertion)
                        {
                            insertSeq[j]=0;
                        }
                    }
                }
            }
            
            
            
        }
        else{ printf("Unknown vt");}
        
        currVT = currVT + cntAlleles;
		samePos = 0;
		if(vt[currVT].position == currentPos)
			samePos = 1;
		else
			currentPos++;
        
        
        
	}
	
	chRef = getNextSymbol(ref);
	
	while (!feof(ref))
	{
		
		for (i=0; i < inBitV; i++)
		{
			if(deleteSeq[i] > 0)
			{
				deleteSeq[i]--;
			}
			else
			{
				writeChar(chRef, buf[i], bufPos[i], linePos[i], resName[i]);
			}
        }
		currentPos++;
		chRef = getNextSymbol(ref);
	}
	
	
    for (i=0; i < inBitV; i++)
	{
        file = fopen(resName[i], "a");
        fwrite(buf[i], 1, bufPos[i], file);
        fclose(file);
	}
	
	
	fclose(ref);
	fclose(vl);
    
    printf("Processed 100%% of reference sequence\n");
    
    
    return 0;
}
