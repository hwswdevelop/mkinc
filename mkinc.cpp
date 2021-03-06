// ****************************************************
// * (C) Evgeny Sobolev 2022 year
// * Simple tool to hide content of included file
// * in the source/binary code. 
// * It is like obufuscation.
// * I used it to hide certificates inside of binary
// * It is fast-writen auto-generation tool
// ****************************************************

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cctype>
#include <time.h>


static constexpr const size_t BufferSize = 16 ;
static constexpr const size_t PasswordSize = 128;

int main( int argc, char* argv[] ){
    if ( argc < 2 ) return -1;

    bool makeTest = (argc > 2);

    char* inFileName = argv[1];

    srand( time(NULL) );
    uint8_t *buf = new uint8_t[ 128 * 1024 ]; // may be exception, buf if disabled is nullptr
    if ( nullptr == buf ) return -1;
    uint8_t* password = new uint8_t[PasswordSize];
    if ( nullptr == password ) return -2;
    memset( password, 0, PasswordSize );

    for(size_t passIndex = 0; passIndex < PasswordSize; passIndex++) {
        password[passIndex] = rand() % ( (1 << (sizeof(uint8_t) * 8)) - 1 );
    }

    FILE* infile = fopen( inFileName, "rb");
    if ( nullptr == infile ) return -3;


    // Find last slash and remove it from Name. Don't use system function..
    size_t slashIndex = 0;
    size_t lastSlashIndex = 0;
    while( inFileName[slashIndex] ) {
        if ( '/' == inFileName[slashIndex] ) {
	    lastSlashIndex = slashIndex;
	}
	slashIndex++;
    }

    char *inFileNameNoPath = inFileName;
    if ( (lastSlashIndex > 0) && ( 0 != inFileName[lastSlashIndex]) ){
        inFileNameNoPath = inFileName + (lastSlashIndex + 1); // Not so good. In fact it shuld be const. FixUp later
    }

    size_t inFileNameNoPathLen = strlen(inFileNameNoPath);
    char* inFileNameInFunc = new char[inFileNameNoPathLen + 1];
    inFileNameInFunc[inFileNameNoPathLen] = '\0';
    strcpy(inFileNameInFunc, inFileNameNoPath);
    for( int index = 0; index < inFileNameNoPathLen; index++){
        inFileNameInFunc[index] = tolower( inFileNameInFunc[index] );
	if ( '.' == inFileNameInFunc[index] ) inFileNameInFunc[index] = '_';
    }
    //inFileNameInFunc[0] = toupper( inFileNameInFunc[0] );
    char* outFileName = new char[inFileNameNoPathLen + 6];
    strcpy(outFileName, inFileNameInFunc);
    if (makeTest) {
        strcat(outFileName, ".cpp");
    } else {
        strcat(outFileName, ".h");
    }

    char* dataName = inFileNameInFunc;
    
    
    FILE* outfile = fopen( outFileName, "w" );
    if ( nullptr == outfile ) return -4;
    

    fprintf( outfile, "\n// This is an autogenerated file, please don't edit it\n\n");
    fprintf( outfile, "// Autogenerated file (C) Evgveny Sobolev\n");
    fprintf( outfile, "// Autogenerator (C) Evgeny Sobolev\n\n");
    if ( !makeTest ) {   
        fprintf( outfile, "#pragma once\n\n");
    } 
    fprintf( outfile, "#include <cstdint>\n");
    fprintf( outfile, "#include <cstddef>\n\n");
    fprintf( outfile, "static uint8_t password_%s[] = {", dataName );

    size_t passIndex = 0;
    while( passIndex < PasswordSize){
       size_t index = passIndex % 8;
       if ( 0 == index ) fprintf( outfile, "\n    " );
       if ( passIndex < (PasswordSize - 1) ){
           fprintf( outfile, "0x%02X, ", password[passIndex] );
       } else {
           fprintf( outfile, "0x%02X", password[passIndex] );
       }
       passIndex++;
    }
    fprintf( outfile, "\n};\n\n");
    size_t passwordStartIndex = rand() % PasswordSize;
    fprintf( outfile, "static constexpr const size_t passwordStartIndex_%s = %ld;\n\n", dataName, passwordStartIndex );


    passIndex = passwordStartIndex;
    size_t readIndex = 0;
    fprintf(outfile, "static uint8_t data_%s[] = {\n", dataName);

    while(!feof(infile)){
       int rdLen = fread( buf, sizeof(uint8_t), BufferSize, infile );
       size_t count = rdLen;
       size_t index = 0;
       while( count-- ) {
           uint8_t val = buf[index] ^ password[passIndex];
	   passIndex = (passIndex + 1) % PasswordSize;
           if ( 0 == (index % 8) ) fprintf( outfile, "\n    " );
	   fprintf(outfile, "0x%02X, ", val);
           index++;
	   readIndex++; 
       }
    }

    fclose(infile);
    fprintf( outfile, "\n};\n\n" );
    fprintf( outfile, "static constexpr const size_t dataSize_%s = %ld;\n\n", dataName, readIndex );


    fprintf( outfile, "size_t getData_%s( uint8_t* buf, size_t maxSize ) {\n", dataName );
    fprintf( outfile, "    if ( 0 == maxSize ) return dataSize_%s;\n", dataName);
    fprintf( outfile, "    if ( nullptr == buf ) return 0;\n");
    fprintf( outfile, "    size_t retSize = (maxSize < dataSize_%s) ? maxSize : dataSize_%s;\n", dataName, dataName);
    fprintf( outfile, "    size_t index = 0;\n");
    fprintf( outfile, "    size_t passIndex = passwordStartIndex_%s;\n", dataName);
    fprintf( outfile, "    while( index < retSize ) {\n");
    fprintf( outfile, "        buf[index] = data_%s[index] ^ password_%s[passIndex];\n", dataName, dataName);
    fprintf( outfile, "        passIndex = (passIndex + 1) %% sizeof(password_%s);\n", dataName);
    fprintf( outfile, "        index++;\n");
    fprintf( outfile, "    };\n");
    fprintf( outfile, "    return retSize;\n");
    fprintf( outfile, "};\n");

    // Dump test
    if ( makeTest ){
        fprintf( outfile, "\n");
        fprintf( outfile, "#include <cstdio>\n");
        fprintf( outfile, "\n");
        fprintf( outfile, "int main(int argc, char* argv[]){\n");
        fprintf( outfile, "    size_t size = getData_%s(nullptr, 0);\n", dataName);
        fprintf( outfile, "    if ( 0 == size ) return 0;\n");
        fprintf( outfile, "    char* buffer = new char[size + 1];\n");
        fprintf( outfile, "    buffer[size] = 0;\n");
        fprintf( outfile, "    size = getData_%s( reinterpret_cast<uint8_t*>(buffer), size );\n", dataName);
        fprintf( outfile, "    printf(\"Data ( %%ld bytes ):\\n%%s\\n\", size, buffer );\n");
        fprintf( outfile, "    return 0;\n");
        fprintf( outfile, "}\n");
    }

    fprintf( outfile, "\n\n");
    fclose(outfile); 
}

