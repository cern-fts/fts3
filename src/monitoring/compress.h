#ifndef compress_h
#define compress_h


enum CompressionScheme
 {
    NO_COMPRESSION,

    ZLIB_COMPRESSION,

    BZIP2_COMPRESSION

};


std::string compress( const std::string& data );

std::string decompress(const std::string& data);

std::string

compress1

(

        const std::string& data,

        CompressionScheme scheme

        );
	
	
std::string

decompress1

(

        const std::string& data,

        CompressionScheme scheme

        );	

#endif
