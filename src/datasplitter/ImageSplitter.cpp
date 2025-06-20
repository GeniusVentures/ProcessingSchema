#include <datasplitter/imagesplitter.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <openssl/sha.h> // For SHA256_DIGEST_LENGTH
#include <gsl/span>

namespace sgns::sgprocessing
{
    ImageSplitter::ImageSplitter( const std::vector<uint8_t> &buffer,
                                  uint64_t                    blockstride,
                                  uint64_t                    blocklinestride,
                                  uint64_t                    blocklen,
                                  int                         channels ) :
        blockstride_( blockstride ), blocklinestride_( blocklinestride ), blocklen_( blocklen ), channels_( channels )
    {
        // Set inputImage and imageSize from the provided buffer
        //inputImage = reinterpret_cast<const unsigned char*>(buffer.data());

        inputImage = reinterpret_cast<const unsigned char *>( buffer.data() );
        imageSize  = buffer.size();
        SplitImageData();
    }

    std::vector<uint8_t> ImageSplitter::GetPart( int part )
    {
        return splitparts_.at( part );
    }

    size_t ImageSplitter::GetPartByCid( const std::vector<uint8_t> &cid ) const
    {
        //Find the index of cid in cids_
        auto it = std::find( cids_.begin(), cids_.end(), cid );
        if ( it == cids_.end() )
        {
            //CID not found
            return -1;
        }

        //Find index in splitparts_ corresponding to cid
        size_t index = std::distance( cids_.begin(), it );

        //Return the data
        if ( index < splitparts_.size() )
        {
            return index;
        }
        //Index out of range
        return -1;
    }

    void ImageSplitter::SplitImageData()
    {
        // Check if imageSize is evenly divisible by blocklen_
        if ( imageSize % blocklen_ != 0 )
        {
            throw std::invalid_argument( "Image size is not evenly divisible by block length" );
        }

        for ( uint64_t i = 0; i < imageSize; i += blocklen_ )
        {
            std::vector<uint8_t> chunkBuffer( blocklen_ );
            int      rowsdone      = ( i / ( blocklen_ * ( ( blockstride_ + blocklinestride_ ) / blockstride_ ) ) );
            uint32_t bufferoffset  = 0 + ( i / blocklen_ * blockstride_ );
            bufferoffset          -= ( blockstride_ + blocklinestride_ ) * rowsdone;
            bufferoffset          += rowsdone * ( blocklen_ * ( ( blockstride_ + blocklinestride_ ) / blockstride_ ) );
            //std::cout << "buffer offset:  " << bufferoffset << std::endl;
            for ( uint64_t size = 0; size < blocklen_; size += blockstride_ )
            {
                auto chunkData = inputImage + bufferoffset;
                std::memcpy( chunkBuffer.data() + ( size ), chunkData, blockstride_ );
                bufferoffset += blockstride_ + blocklinestride_;
            }
            //std::string filename = "chunk_" + std::to_string(i) + ".png";
            //int result = stbi_write_png(filename.c_str(), blockstride_ / 4, blocklen_ / blockstride_, 4, chunkBuffer.data(), blockstride_);
            //if (!result) {
            //    std::cerr << "Error writing PNG file: " << filename << "\n";
            //}
            splitparts_.push_back( chunkBuffer );
            chunkWidthActual_.push_back( blockstride_ / channels_ );
            chunkHeightActual_.push_back( blocklen_ / blockstride_ );
            gsl::span<const uint8_t> byte_span( chunkBuffer );
            std::vector<uint8_t>     shahash( SHA256_DIGEST_LENGTH );
            unsigned int             digest_len = 0;

            EVP_MD_CTX *ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex( ctx, EVP_sha256(), NULL );
            EVP_DigestUpdate( ctx, chunkBuffer.data(), chunkBuffer.size() );
            EVP_DigestFinal_ex( ctx, shahash.data(), &digest_len );
            EVP_MD_CTX_free( ctx );

            cids_.emplace_back( shahash );
        }
    }

    uint32_t ImageSplitter::GetPartSize( int part ) const
    {
        return splitparts_.at( part ).size();
    }

    uint32_t ImageSplitter::GetPartStride( int part ) const
    {
        return chunkWidthActual_.at( part );
    }

    int ImageSplitter::GetPartWidthActual( int part ) const
    {
        return chunkWidthActual_.at( part );
    }

    int ImageSplitter::GetPartHeightActual( int part ) const
    {
        return chunkHeightActual_.at( part );
    }

    size_t ImageSplitter::GetPartCount() const
    {
        return splitparts_.size();
    }

    size_t ImageSplitter::GetImageSize() const
    {
        return imageSize;
    }

    std::vector<uint8_t> ImageSplitter::GetPartCID( int part ) const
    {
        return cids_.at( part );
    }
}
