/*
        COMPRESS_INTEGER_QMX_IMPROVED.H
        -------------------------------
        Copyright (c) 2014-2017 Andrew Trotman
        Released under the 2-clause BSD license
   (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
/*!
        @file
        @brief Improved QMX Compression.
        @author Andrew Trotman
        @copyright 2016 Andrew Trotman
*/
#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

#include <emmintrin.h>
#include <smmintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>

#define QMX_SOURCE_OFFSET 0
#define QMX_KEYS_OFFSET 1
#define QMX_DESTINATION_OFFSET 2

namespace QMX
{
  alignas(16) static uint32_t static_mask_21[] = {
      0x1fffff, 0x1fffff, 0x1fffff, 0x1fffff}; ///< AND mask for 21-bit integers
  alignas(16) static uint32_t static_mask_12[] = {
      0xfff, 0xfff, 0xfff, 0xfff}; ///< AND mask for 12-bit integers
  alignas(16) static uint32_t static_mask_10[] = {
      0x3ff, 0x3ff, 0x3ff, 0x3ff}; ///< AND mask for 10-bit integers
  alignas(16) static uint32_t static_mask_9[] = {
      0x1ff, 0x1ff, 0x1ff, 0x1ff}; ///< AND mask for 9-bit integers
  alignas(16) static uint32_t static_mask_7[] = {
      0x7f, 0x7f, 0x7f, 0x7f}; ///< AND mask for 7-bit integers
  alignas(16) static uint32_t static_mask_6[] = {
      0x3f, 0x3f, 0x3f, 0x3f}; ///< AND mask for 6-bit integers
  alignas(16) static uint32_t static_mask_5[] = {
      0x1f, 0x1f, 0x1f, 0x1f}; ///< AND mask for 5-bit integers
  alignas(16) static uint32_t static_mask_4[] = {
      0x0f, 0x0f, 0x0f, 0x0f}; ///< AND mask for 4-bit integers
  alignas(16) static uint32_t static_mask_3[] = {
      0x07, 0x07, 0x07, 0x07}; ///< AND mask for 3-bit integers
  alignas(16) static uint32_t static_mask_2[] = {
      0x03, 0x03, 0x03, 0x03}; ///< AND mask for 2-bit integers
  alignas(16) static uint32_t static_mask_1[] = {
      0x01, 0x01, 0x01, 0x01}; ///< AND mask for 1-bit integers

  /*
          CLASS COMPRESS_INTEGER_QMX_IMPROVED
          -----------------------------------
  */
  /*!
          @brief QMX compression improved (smaller and faster to decode)
          @details  Trotman & Lin  describe several improvements to the QMX codex
     in:

          A. Trotman, J. Lin (2016), In Vacuo and In Situ Evaluation of SIMD
     Codecs, Proceedings of The 21st Australasian Document Computing Symposium
     (ADCS 2016

          including removal of the vbyte encoded length from the end of the
     encoded sequence.  This version of QMX is the original QMX with that
     improvement added, but none of the other imprivements suggested by Trotman &
     Lin.  This makes the encoded sequence smaller, and faster to decode, than any
     of the other alrernatives suggested.  It does not include the code to prevent
     read and write overruns from the encoded string and into the decode buffer.
     To account for overwrites make sure the decode-into buffer is at least 256
     integers larger than required.  To prevent over-reads from the encoded string
     make sure that that string is at least 16 bytes longer than needed.

          At the request of Matthias Petri (University of Melbourne), the code no
     longer requires SIMD-word alignment to decode (the read and write
          instructions have been changed from aligned to unaligned since Intel
     made them faster).

          For details on the original QMX encoding see:

          A. Trotman (2014), Compression, SIMD, and Postings Lists, Proceedings of
     the 19th Australasian Document Computing Symposium (ADCS 2014)
  */

  static uint8_t bits_needed_for(uint32_t value)
  {
    if (value == 0x01)
      return 0;
    else if (value <= 0x01)
      return 1;
    else if (value <= 0x03)
      return 2;
    else if (value <= 0x07)
      return 3;
    else if (value <= 0x0F)
      return 4;
    else if (value <= 0x1F)
      return 5;
    else if (value <= 0x3F)
      return 6;
    else if (value <= 0x7F)
      return 7;
    else if (value <= 0xFF)
      return 8;
    else if (value <= 0x1FF)
      return 9;
    else if (value <= 0x3FF)
      return 10;
    else if (value <= 0xFFF)
      return 12;
    else if (value <= 0xFFFF)
      return 16;
    else if (value <= 0x1FFFFF)
      return 21;
    else
      return 32;
  }

  /*
                  STRUCT TYPE_AND_INTEGERS
                  ------------------------
          */
  /*!
          @brief a tuple of the numner of integers and which selector represents
     that.
  */
  struct type_and_integers
  {
    uint8_t type;      ///< The selector
    uint32_t integers; ///< The number of integers in a word
  };

  /*
          TABLE[]
          -------
  */
  /*!
          @brief Each row stores the selector number and the number of integers
     where the index is the number of bits
  */
  static const type_and_integers table[] = {
      {0, 256}, // size_in_bits == 0;
      {1, 128}, // size_in_bits == 1;
      {2, 64},  // size_in_bits == 2;
      {3, 40},  // size_in_bits == 3;
      {4, 32},  // size_in_bits == 4;
      {5, 24},  // size_in_bits == 5;
      {6, 20},  // size_in_bits == 6;
      {7, 36},  // size_in_bits == 7;  256-bits
      {8, 16},  // size_in_bits == 8;
      {9, 28},  // size_in_bits == 9;  256-bits
      {10, 12}, // size_in_bits == 10;
      {0, 0},
      {11, 20}, // size_in_bits == 12;
      {0, 0},
      {0, 0},
      {0, 0},
      {12, 8}, // size_in_bits == 16;
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {13, 12}, // size_in_bits == 21;	256-bits
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {14, 4}, // size_in_bits == 32;
  };

  /*
          MAXIMUM()
          ---------
  */
  /*!
          @brief Compute the minimum of two numbers
  */
  template <class T>
  T maximum(T a, T b) { return a > b ? a : b; }

  /*
          MAXIMUM()
          ---------
  */
  /*!
          @brief Compute the maximum of two numbers
  */
  template <class T>
  T maximum(T a, T b, T c, T d)
  {
    return maximum(maximum(a, b), maximum(c, d));
  }

  class QMXCodec
  {
  private:
    uint8_t *length_buffer;        ///< Stores the number of bits needed to compress each
                                   ///< integer
    uint64_t length_buffer_length; ///< The length of length_buffer
    uint32_t *full_length_buffer;  ///< If the run_length is too short then 0-pad
                                   ///< into this buffer

  private:
    /*
            COMPRESS_INTEGER_QMX_IMPROVED::WRITE_OUT()
            ------------------------------------------
    */
    /*!
            @brief Encode and write out the sequence into the buffer
            @param buffer [in] where to write the encoded sequence
            @param source [in] the integer sequence to encode
            @param raw_count [in] the numnber of integers to encode
            @param size_in_bits [in] the size, in bits, of the largest integer
            @param length_buffer [in] the length of buffer, in bytes
    */
    /*
          COMPRESS_INTEGER_QMX_IMPROVED::WRITE_OUT()
          ------------------------------------------
          write a sequence into the destination buffer
  */
    void write_out(uint8_t **buffer, uint32_t *source, uint32_t raw_count,
                   uint32_t size_in_bits, uint8_t **length_buffer)
    {
      uint32_t current;
      uint8_t *destination = *buffer;
      uint8_t *key_store = *length_buffer;
      uint32_t sequence_buffer[4];
      uint32_t instance, value;
      uint8_t type;
      uint32_t count;

      if (size_in_bits > 32)
        exit(printf("Can't compress into integers of size %d bits\n",
                    (int)size_in_bits)); // LCOV_EXCL_LINE
      type = table[size_in_bits].type;
      count = (raw_count + table[size_in_bits].integers - 1) /
              table[size_in_bits].integers;

      uint32_t *end = source + raw_count;

      while (count > 0)
      {
        uint32_t batch = count > 16 ? 16 : count;
        *key_store++ = (type << 4) | (~(batch - 1) & 0x0F);

        count -= batch;

        /*
                0-pad if there aren't enough integers in the source buffer.
        */
        if (source + table[size_in_bits].integers * batch >
            end)
        { // must 0-pad to prevent read overflow in input buffer
          auto new_end = full_length_buffer + (end - source);
          std::fill(new_end, new_end + table[size_in_bits].integers, 0);
          std::copy(source, end, full_length_buffer);
          end = new_end;
          source = full_length_buffer;
        }

        for (current = 0; current < batch; current++)
        {
          switch (size_in_bits)
          {
          case 0: // 0 bits per integer (i.e. a long sequence of zeros)
            /*
                    In this case we don't need to store a 4 byte integer because
               its implicit
            */
            source += 256;
            break;
          case 1: // 1 bit per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 128; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 1);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 128;
            break;
          case 2: // 2 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 64; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 2);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 64;
            break;
          case 3: // 3 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 40; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 3);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 40;
            break;
          case 4: // 4 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 32; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 4);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 32;
            break;
          case 5: // 5 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 24; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 5);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 24;
            break;
          case 6: // 6 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 20; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 6);
            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 20;
            break;
          case 7: // 7 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 20; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 7);
            memcpy(destination, sequence_buffer, 16);
            destination += 16;

            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 16; value < 20; value++)
              sequence_buffer[value & 0x03] |= source[value] >> 4;
            for (value = 20; value < 36; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << (((value - 20) / 4) * 7 + 3);
            memcpy(destination, sequence_buffer, 16);

            destination += 16;
            source += 36; // 36 in a double 128-bit word
            break;
          case 8: // 8 bits per integer
            for (instance = 0; instance < 16 && source < end; instance++)
              *destination++ = (uint8_t)*source++;
            break;
          case 9: // 9 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 16; value++)
              sequence_buffer[value & 0x03] |= source[value] << ((value / 4) * 9);
            memcpy(destination, sequence_buffer, 16);
            destination += 16;

            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 12; value < 16; value++)
              sequence_buffer[value & 0x03] |= source[value] >> 5;
            for (value = 16; value < 28; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << (((value - 16) / 4) * 9 + 4);
            memcpy(destination, sequence_buffer, 16);

            destination += 16;
            source += 28; // 28 in a double 128-bit word
            break;
          case 10: // 10 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 12; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << ((value / 4) * 10);

            memcpy(destination, sequence_buffer, 16);
            destination += 16;
            source += 12;
            break;
          case 12: // 12 bit integers
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 12; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << ((value / 4) * 12);
            memcpy(destination, sequence_buffer, 16);
            destination += 16;

            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 8; value < 12; value++)
              sequence_buffer[value & 0x03] |= source[value] >> 8;
            for (value = 12; value < 20; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << (((value - 12) / 4) * 12 + 8);
            memcpy(destination, sequence_buffer, 16);

            destination += 16;
            source += 20; // 20 in a double 128-bit word
            break;
          case 16: // 16 bits per integer
            for (instance = 0; instance < 8 && source < end; instance++)
            {
              *reinterpret_cast<uint16_t *>(destination) = (uint16_t)*source++;
              destination += 2;
            }
            break;
          case 21: // 21 bits per integer
            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 0; value < 8; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << ((value / 4) * 21);
            memcpy(destination, sequence_buffer, 16);
            destination += 16;

            memset(sequence_buffer, 0, sizeof(sequence_buffer));
            for (value = 4; value < 8; value++)
              sequence_buffer[value & 0x03] |= source[value] >> 11;
            for (value = 8; value < 12; value++)
              sequence_buffer[value & 0x03] |= source[value]
                                               << (((value - 8) / 4) * 21 + 11);
            memcpy(destination, sequence_buffer, 16);

            destination += 16;
            source += 12; // 12 in a double 128-bit word
            break;
          case 32: // 32 bits per integer
            for (instance = 0; instance < 4 && source < end; instance++)
            {
              *reinterpret_cast<uint32_t *>(destination) = (uint32_t)*source++;
              destination += 4;
            }
            break;
          }
        }
      }
      *buffer = destination;
      *length_buffer = key_store;
    }

  public:
    typedef uint32_t integer; ///< This class and descendants will work on
                              ///< integers of this size.  Do not change without
                              ///< also changing
                              ///< JASS_COMPRESS_INTEGER_BITS_PER_INTEGER

    /*
            COMPRESS_INTEGER_QMX_IMPROVED::COMPRESS_INTEGER_QMX_IMPROVED()
            --------------------------------------------------------------
    */
    /*!
            @brief Constructor
    */
    QMXCodec()
        : length_buffer(nullptr), length_buffer_length(0),
          full_length_buffer(new uint32_t[256 * 16])
    {
      /* Nothing */
    }

    /*
            COMPRESS_INTEGER_QMX_IMPROVED::~COMPRESS_INTEGER_QMX_IMPROVED()
            ---------------------------------------------------------------
    */
    /*!
            @brief Destructor
    */
    virtual ~QMXCodec()
    {
      delete[] length_buffer;
      delete[] full_length_buffer;
    }

    /*
            COMPRESS_INTEGER_QMX_IMPROVED::ENCODE()
            ---------------------------------------
    */
    /*!
            @brief Encode a sequence of integers returning the number of bytes
       used for the encoding, or 0 if the encoded sequence doesn't fit in the
       buffer.
            @param encoded [out] The sequence of bytes that is the encoded
       sequence.
            @param encoded_buffer_length [in] The length (in bytes) of the output
       buffer, encoded.
            @param source [in] The sequence of integers to encode.
            @param source_integers [in] The length (in integers) of the source
       buffer.
            @return The number of bytes used to encode the integer sequence, or 0
       on error (i.e. overflow).
    */
    size_t encode(void *into_as_void, size_t encoded_buffer_length,
                  const integer *source, size_t source_integers)
    {
      uint32_t *into = static_cast<uint32_t *>(into_as_void);
      const uint32_t WASTAGE = 512;
      uint8_t *current_length, *destination = (uint8_t *)into, *keys;
      uint32_t *current, run_length, bits, wastage;
      uint32_t block, largest;

      /*
              make sure we have enough room to store the lengths
      */
      if (length_buffer_length < source_integers)
      {
        delete[] length_buffer;
        length_buffer = new uint8_t[(size_t)((length_buffer_length = source_integers) + WASTAGE)];
      }

      /*
              Get the lengths of the integers
      */
      current_length = length_buffer;
      for (current = (uint32_t *)source; current < source + source_integers;
           current++)
        *current_length++ = bits_needed_for(*current);

      /*
              Shove a bunch of 0 length integers on the end to allow for overflow
      */
      for (wastage = 0; wastage < WASTAGE; wastage++)
        *current_length++ = 0;

      /*
              Process the lengths.  To maximise SSE throughput we need each write
         to be 128-bit (4*32-bit) alignned and therefore we need each compress
         "block" to be the same size where a compress "block" is a set of four
         encoded integers starting on a 4-integer boundary.
      */
      for (current_length = length_buffer;
           current_length < length_buffer + source_integers + 4;
           current_length += 4)
        *current_length = *(current_length + 1) = *(current_length + 2) =
            *(current_length + 3) =
                maximum(*current_length, *(current_length + 1),
                        *(current_length + 2), *(current_length + 3));

      /*
              This code makes sure we can do aligned reads, promoting to larger
         integers if necessary
      */
      current_length = length_buffer;
      while (current_length < length_buffer + source_integers)
      {
        /** 
          * - If there are fewer than 16 values remaining and they all fit into 8-bits,
          * then its smaller than storing stripes. 
          * - If there are fewer than 8 values remaining and they all fit into 16-bits (2-bits each), 
          * then its smaller than storing stripes.
          * - If there are fewer than 4 values remaining and they all fit into 32-bits (8-bits each), 
          * then its smaller than storing stripes.
        **/
        if (source_integers - (current_length - length_buffer) < 4)
        {
          largest = 0;
          for (block = 0; block < 8; block++)
            largest = maximum((uint8_t)largest, *(current_length + block));
          if (largest <= 8)
            for (block = 0; block < 8; block++)
              *(current_length + block) = 8;
          else if (largest <= 16)
            for (block = 0; block < 8; block++)
              *(current_length + block) = 16;
          else if (largest <= 32)
            for (block = 0; block < 8; block++)
              *(current_length + block) = 32;
        }
        else if (source_integers - (current_length - length_buffer) < 8)
        {
          largest = 0;
          for (block = 0; block < 8; block++)
            largest = maximum((uint8_t)largest, *(current_length + block));
          if (largest <= 8)
            for (block = 0; block < 8; block++)
              *(current_length + block) = 8;
          else if (largest <= 16)
            for (block = 0; block < 16; block++)
              *(current_length + block) = 16;
        }
        else if (source_integers - (current_length - length_buffer) < 16)
        {
          largest = 0;
          for (block = 0; block < 16; block++)
            largest = maximum((uint8_t)largest, *(current_length + block));
          if (largest <= 8)
            for (block = 0; block < 16; block++)
              *(current_length + block) = 8;
        }
        /*
                Otherwise we have the standard rules for a block
        */
        switch (*current_length)
        {
        case 0:
          for (block = 0; block < 256; block += 4)
            if (*(current_length + block) > 0)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 1; // promote
          if (*current_length == 0) 
          {
            for (block = 0; block < 256; block++)
              current_length[block] = 0;
            current_length += 256;
          }
          break;
        case 1:
          for (block = 0; block < 128; block += 4)
            if (*(current_length + block) > 1)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 2; // promote
          if (*current_length == 1)
          {
            for (block = 0; block < 128; block++)
              current_length[block] = 1;
            current_length += 128;
          }
          break;
        case 2:
          for (block = 0; block < 64; block += 4)
            if (*(current_length + block) > 2)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 3; // promote
          if (*current_length == 2)
          {
            for (block = 0; block < 64; block++)
              current_length[block] = 2;
            current_length += 64;
          }
          break;
        case 3:
          for (block = 0; block < 40; block += 4)
            if (*(current_length + block) > 3)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 4; // promote
          if (*current_length == 3)
          {
            for (block = 0; block < 40; block++)
              current_length[block] = 3;
            current_length += 40;
          }
          break;
        case 4:
          for (block = 0; block < 32; block += 4)
            if (*(current_length + block) > 4)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 5; // promote
          if (*current_length == 4)
          {
            for (block = 0; block < 32; block++)
              current_length[block] = 4;
            current_length += 32;
          }
          break;
        case 5:
          for (block = 0; block < 24; block += 4)
            if (*(current_length + block) > 5)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 6; // promote
          if (*current_length == 5)
          {
            for (block = 0; block < 24; block++)
              current_length[block] = 5;
            current_length += 24;
          }
          break;
        case 6:
          for (block = 0; block < 20; block += 4)
            if (*(current_length + block) > 6)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 7; // promote
          if (*current_length == 6)
          {
            for (block = 0; block < 20; block++)
              current_length[block] = 6;
            current_length += 20;
          }
          break;
        case 7:
          for (block = 0; block < 36; block += 4) // 36 in a double 128-bit word
            if (*(current_length + block) > 7)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 8; // promote
          if (*current_length == 7)
          {
            for (block = 0; block < 36; block++)
              current_length[block] = 7;
            current_length += 36;
          }
          break;
        case 8:
          for (block = 0; block < 16; block += 4)
            if (*(current_length + block) > 8)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 9; // promote
          if (*current_length == 8)
          {
            for (block = 0; block < 16; block++)
              current_length[block] = 8;
            current_length += 16;
          }
          break;
        case 9:
          for (block = 0; block < 28; block += 4) // 28 in a double 128-bit word
            if (*(current_length + block) > 9)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 10; // promote
          if (*current_length == 9)
          {
            for (block = 0; block < 28; block++)
              current_length[block] = 9;
            current_length += 28;
          }
          break;
        case 10:
          for (block = 0; block < 12; block += 4)
            if (*(current_length + block) > 10)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 12; // promote
          if (*current_length == 10)
          {
            for (block = 0; block < 12; block++)
              current_length[block] = 10;
            current_length += 12;
          }
          break;
        case 12:
          for (block = 0; block < 20; block += 4) // 20 in a double 128-bit word
            if (*(current_length + block) > 12)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 16; // promote
          if (*current_length == 12)
          {
            for (block = 0; block < 20; block++)
              current_length[block] = 12;
            current_length += 20;
          }
          break;
        case 16:
          for (block = 0; block < 8; block += 4)
            if (*(current_length + block) > 16)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 21; // promote
          if (*current_length == 16)
          {
            for (block = 0; block < 8; block++)
              current_length[block] = 16;
            current_length += 8;
          }
          break;
        case 21:
          for (block = 0; block < 12; block += 4) // 12 in a double 128-bit word
            if (*(current_length + block) > 21)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) = 32; // promote
          if (*current_length == 21)
          {
            for (block = 0; block < 12; block++)
              current_length[block] = 21;
            current_length += 12;
          }
          break;
        case 32:
          for (block = 0; block < 4; block += 4)
            if (*(current_length + block) > 32)
              *current_length = *(current_length + 1) = *(current_length + 2) =
                  *(current_length + 3) =
                      64; // LCOV_EXCL_LINE  // can't happen	// promote
          if (*current_length == 32)
          {
            for (block = 0; block < 4; block++)
              current_length[block] = 32;
            current_length += 4;
          }
          break;
        default:
          exit(printf(
              "Selecting on a non whole power of 2, must exit\n")); // LCOV_EXCL_LINE
          break;                                                    // LCOV_EXCL_LINE
        }
      }

      /*
              We can now compress based on the lengths in length_buffer
      */
      run_length = 1;
      bits = length_buffer[0];
      keys = length_buffer; // we're going to re-use the length_buffer because it
                            // can't overlap and this saves a double malloc
      for (current = (uint32_t *)source + 1; current < source + source_integers;
           current++)
      {
        uint32_t new_needed = length_buffer[current - source];
        if (new_needed == bits)
          run_length++;
        else
        {
          write_out(&destination, (uint32_t *)current - run_length, run_length,
                    bits, &keys);
          bits = new_needed;
          run_length = 1;
        }
      }
      write_out(&destination, (uint32_t *)current - run_length, run_length, bits,
                &keys);

      /*
              Copy the lengths to the end, backwards
      */
      uint8_t *from = length_buffer + (keys - length_buffer) - 1;
      uint8_t *to = destination;
      for (uint32_t pos = 0; pos < keys - length_buffer; pos++)
        *to++ = *from--;
      destination += keys - length_buffer;

      /*
              Compute the length (in bytes)
      */
      return destination - (uint8_t *)into; // return length in bytes
    }

    /*
            COMPRESS_INTEGER_QMX_IMPROVED::DECODE()
            ---------------------------------------
    */
    /*!
            @brief Decode a sequence of integers encoded with this codex.
            @param decoded [out] The sequence of decoded integers.
            @param integers_to_decode [in] The minimum number of integers to
       decode (it may decode more).
            @param source [in] The encoded integers.
            @param source_length [in] The length (in bytes) of the source buffer.
    */
   std::array<size_t, 3> decode(integer *to, size_t destination_integers, const void *source,
                const size_t len, const std::array<size_t, 3> offsets)
    {
      size_t  source_offset = offsets[QMX_SOURCE_OFFSET],
              keys_offset = offsets[QMX_KEYS_OFFSET],
              to_offset = offsets[QMX_DESTINATION_OFFSET];

      std::cout << "* source_offset = " << source_offset << "; keys_offset = " << keys_offset << std::endl;
      
      __m128i byte_stream, byte_stream_2, tmp, tmp2, mask_21, mask_12, mask_10,
          mask_9, mask_7, mask_6, mask_5, mask_4, mask_3, mask_2, mask_1;
      uint8_t *in = (uint8_t *)source + source_offset; // to be returned: in - ((uint8_t *)source)
      uint8_t *keys = ((uint8_t *)source) + len - 1 - keys_offset; // to be returned:  ((uint8_t *)source) + len - 1 - keys
      integer* destination_base = to;
      to = to + to_offset; // to be returned: to - destination_base

      mask_21 = _mm_loadu_si128((__m128i *)static_mask_21);
      mask_12 = _mm_loadu_si128((__m128i *)static_mask_12);
      mask_10 = _mm_loadu_si128((__m128i *)static_mask_10);
      mask_9 = _mm_loadu_si128((__m128i *)static_mask_9);
      mask_7 = _mm_loadu_si128((__m128i *)static_mask_7);
      mask_6 = _mm_loadu_si128((__m128i *)static_mask_6);
      mask_5 = _mm_loadu_si128((__m128i *)static_mask_5);
      mask_4 = _mm_loadu_si128((__m128i *)static_mask_4);
      mask_3 = _mm_loadu_si128((__m128i *)static_mask_3);
      mask_2 = _mm_loadu_si128((__m128i *)static_mask_2);
      mask_1 = _mm_loadu_si128((__m128i *)static_mask_1);


      while (in <= keys) // <= because there can be a boundary case where the
                         // final key is 255*0 bit integers
      {
        std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << std::endl;
        switch (*keys--)
        {
        case 0x00:

          std::cout << "Case 0x00 (" << std::dec << 0x00 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x01:

          std::cout << "Case 0x01 (" << std::dec << 0x01 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x02:

          std::cout << "Case 0x02 (" << std::dec << 0x02 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x03:

          std::cout << "Case 0x03 (" << std::dec << 0x03 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x04:

          std::cout << "Case 0x04 (" << std::dec << 0x04 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x05:

          std::cout << "Case 0x05 (" << std::dec << 0x05 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x06:

          std::cout << "Case 0x06 (" << std::dec << 0x06 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x07:

          std::cout << "Case 0x07 (" << std::dec << 0x07 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x08:

          std::cout << "Case 0x08 (" << std::dec << 0x08 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x09:

          std::cout << "Case 0x09 (" << std::dec << 0x09 << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0a:

          std::cout << "Case 0x0a (" << std::dec << 0x0a << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0b:

          std::cout << "Case 0x0b (" << std::dec << 0x0b << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0c:

          std::cout << "Case 0x0c (" << std::dec << 0x0c << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0d:

          std::cout << "Case 0x0d (" << std::dec << 0x0d << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0e:

          std::cout << "Case 0x0e (" << std::dec << 0x0e << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
        case 0x0f:

          std::cout << "Case 0x0f (" << std::dec << 0x0f << ")" << std::endl;

          tmp = _mm_loadu_si128((__m128i *)static_mask_1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
          to += 256;
          
          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x10:

          std::cout << "Case 0x10 (" << std::dec << 0x10 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x11:

          std::cout << "Case 0x11 (" << std::dec << 0x11 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x12:

          std::cout << "Case 0x12 (" << std::dec << 0x12 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x13:

          std::cout << "Case 0x13 (" << std::dec << 0x13 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x14:

          std::cout << "Case 0x14 (" << std::dec << 0x14 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x15:

          std::cout << "Case 0x15 (" << std::dec << 0x15 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x16:

          std::cout << "Case 0x16 (" << std::dec << 0x16 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x17:

          std::cout << "Case 0x17 (" << std::dec << 0x17 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x18:

          std::cout << "Case 0x18 (" << std::dec << 0x18 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x19:

          std::cout << "Case 0x19 (" << std::dec << 0x19 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1a:

          std::cout << "Case 0x1a (" << std::dec << 0x1a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1b:

          std::cout << "Case 0x1b (" << std::dec << 0x1b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1c:

          std::cout << "Case 0x1c (" << std::dec << 0x1c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1d:

          std::cout << "Case 0x1d (" << std::dec << 0x1d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1e:

          std::cout << "Case 0x1e (" << std::dec << 0x1e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;
        case 0x1f:

          std::cout << "Case 0x1f (" << std::dec << 0x1f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
                           _mm_and_si128(byte_stream, mask_1));
          byte_stream = _mm_srli_epi64(byte_stream, 1);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
                           _mm_and_si128(byte_stream, mask_1));
          in += 16;
          to += 128;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x20:

          std::cout << "Case 0x20 (" << std::dec << 0x20 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x21:

          std::cout << "Case 0x21 (" << std::dec << 0x21 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x22:

          std::cout << "Case 0x22 (" << std::dec << 0x22 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x23:

          std::cout << "Case 0x23 (" << std::dec << 0x23 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x24:

          std::cout << "Case 0x24 (" << std::dec << 0x24 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x25:

          std::cout << "Case 0x25 (" << std::dec << 0x25 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x26:

          std::cout << "Case 0x26 (" << std::dec << 0x26 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x27:

          std::cout << "Case 0x27 (" << std::dec << 0x27 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x28:

          std::cout << "Case 0x28 (" << std::dec << 0x28 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x29:

          std::cout << "Case 0x29 (" << std::dec << 0x29 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2a:

          std::cout << "Case 0x2a (" << std::dec << 0x2a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2b:

          std::cout << "Case 0x2b (" << std::dec << 0x2b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2c:

          std::cout << "Case 0x2c (" << std::dec << 0x2c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2d:

          std::cout << "Case 0x2d (" << std::dec << 0x2d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2e:

          std::cout << "Case 0x2e (" << std::dec << 0x2e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;
        case 0x2f:

          std::cout << "Case 0x2f (" << std::dec << 0x2f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
                           _mm_and_si128(byte_stream, mask_2));
          byte_stream = _mm_srli_epi64(byte_stream, 2);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
                           _mm_and_si128(byte_stream, mask_2));
          in += 16;
          to += 64;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x30:

          std::cout << "Case 0x30 (" << std::dec << 0x30 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x31:

          std::cout << "Case 0x31 (" << std::dec << 0x31 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x32:

          std::cout << "Case 0x32 (" << std::dec << 0x32 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x33:

          std::cout << "Case 0x33 (" << std::dec << 0x33 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x34:

          std::cout << "Case 0x34 (" << std::dec << 0x34 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x35:

          std::cout << "Case 0x35 (" << std::dec << 0x35 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x36:

          std::cout << "Case 0x36 (" << std::dec << 0x36 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x37:

          std::cout << "Case 0x37 (" << std::dec << 0x37 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x38:

          std::cout << "Case 0x38 (" << std::dec << 0x38 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x39:

          std::cout << "Case 0x39 (" << std::dec << 0x39 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3a:

          std::cout << "Case 0x3a (" << std::dec << 0x3a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3b:

          std::cout << "Case 0x3b (" << std::dec << 0x3b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3c:

          std::cout << "Case 0x3c (" << std::dec << 0x3c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3d:

          std::cout << "Case 0x3d (" << std::dec << 0x3d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3e:

          std::cout << "Case 0x3e (" << std::dec << 0x3e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;
        case 0x3f:

          std::cout << "Case 0x3f (" << std::dec << 0x3f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_3));
          byte_stream = _mm_srli_epi64(byte_stream, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
                           _mm_and_si128(byte_stream, mask_3));
          in += 16;
          to += 40;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x40:

          std::cout << "Case 0x40 (" << std::dec << 0x40 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x41:

          std::cout << "Case 0x41 (" << std::dec << 0x41 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x42:

          std::cout << "Case 0x42 (" << std::dec << 0x42 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x43:

          std::cout << "Case 0x43 (" << std::dec << 0x43 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x44:

          std::cout << "Case 0x44 (" << std::dec << 0x44 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x45:

          std::cout << "Case 0x45 (" << std::dec << 0x45 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x46:

          std::cout << "Case 0x46 (" << std::dec << 0x46 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x47:

          std::cout << "Case 0x47 (" << std::dec << 0x47 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x48:

          std::cout << "Case 0x48 (" << std::dec << 0x48 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x49:

          std::cout << "Case 0x49 (" << std::dec << 0x49 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4a:

          std::cout << "Case 0x4a (" << std::dec << 0x4a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4b:

          std::cout << "Case 0x4b (" << std::dec << 0x4b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4c:

          std::cout << "Case 0x4c (" << std::dec << 0x4c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4d:

          std::cout << "Case 0x4d (" << std::dec << 0x4d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4e:

          std::cout << "Case 0x4e (" << std::dec << 0x4e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;
        case 0x4f:

          std::cout << "Case 0x4f (" << std::dec << 0x4f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_4));
          byte_stream = _mm_srli_epi64(byte_stream, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_4));
          in += 16;
          to += 32;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x50:

          std::cout << "Case 0x50 (" << std::dec << 0x50 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x51:

          std::cout << "Case 0x51 (" << std::dec << 0x51 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x52:

          std::cout << "Case 0x52 (" << std::dec << 0x52 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x53:

          std::cout << "Case 0x53 (" << std::dec << 0x53 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x54:

          std::cout << "Case 0x54 (" << std::dec << 0x54 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x55:

          std::cout << "Case 0x55 (" << std::dec << 0x55 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x56:

          std::cout << "Case 0x56 (" << std::dec << 0x56 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x57:

          std::cout << "Case 0x57 (" << std::dec << 0x57 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x58:

          std::cout << "Case 0x58 (" << std::dec << 0x58 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x59:

          std::cout << "Case 0x59 (" << std::dec << 0x59 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5a:

          std::cout << "Case 0x5a (" << std::dec << 0x5a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5b:

          std::cout << "Case 0x5b (" << std::dec << 0x5b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5c:

          std::cout << "Case 0x5c (" << std::dec << 0x5c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5d:

          std::cout << "Case 0x5d (" << std::dec << 0x5d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5e:

          std::cout << "Case 0x5e (" << std::dec << 0x5e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;
        case 0x5f:

          std::cout << "Case 0x5f (" << std::dec << 0x5f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_5));
          byte_stream = _mm_srli_epi64(byte_stream, 5);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_5));
          in += 16;
          to += 24;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x60:

          std::cout << "Case 0x60 (" << std::dec << 0x60 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x61:

          std::cout << "Case 0x61 (" << std::dec << 0x61 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x62:

          std::cout << "Case 0x62 (" << std::dec << 0x62 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x63:

          std::cout << "Case 0x63 (" << std::dec << 0x63 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x64:

          std::cout << "Case 0x64 (" << std::dec << 0x64 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x65:

          std::cout << "Case 0x65 (" << std::dec << 0x65 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x66:

          std::cout << "Case 0x66 (" << std::dec << 0x66 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x67:

          std::cout << "Case 0x67 (" << std::dec << 0x67 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x68:

          std::cout << "Case 0x68 (" << std::dec << 0x68 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x69:

          std::cout << "Case 0x69 (" << std::dec << 0x69 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6a:

          std::cout << "Case 0x6a (" << std::dec << 0x6a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6b:

          std::cout << "Case 0x6b (" << std::dec << 0x6b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6c:

          std::cout << "Case 0x6c (" << std::dec << 0x6c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6d:

          std::cout << "Case 0x6d (" << std::dec << 0x6d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6e:

          std::cout << "Case 0x6e (" << std::dec << 0x6e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;
        case 0x6f:

          std::cout << "Case 0x6f (" << std::dec << 0x6f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_6));
          byte_stream = _mm_srli_epi64(byte_stream, 6);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_6));
          in += 16;
          to += 20;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x70:

          std::cout << "Case 0x70 (" << std::dec << 0x70 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x71:

          std::cout << "Case 0x71 (" << std::dec << 0x71 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x72:

          std::cout << "Case 0x72 (" << std::dec << 0x72 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x73:

          std::cout << "Case 0x73 (" << std::dec << 0x73 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x74:

          std::cout << "Case 0x74 (" << std::dec << 0x74 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x75:

          std::cout << "Case 0x75 (" << std::dec << 0x75 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x76:

          std::cout << "Case 0x76 (" << std::dec << 0x76 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x77:

          std::cout << "Case 0x77 (" << std::dec << 0x77 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x78:

          std::cout << "Case 0x78 (" << std::dec << 0x78 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x79:

          std::cout << "Case 0x79 (" << std::dec << 0x79 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7a:

          std::cout << "Case 0x7a (" << std::dec << 0x7a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7b:

          std::cout << "Case 0x7b (" << std::dec << 0x7b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7c:

          std::cout << "Case 0x7c (" << std::dec << 0x7c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7d:

          std::cout << "Case 0x7d (" << std::dec << 0x7d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7e:

          std::cout << "Case 0x7e (" << std::dec << 0x7e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;
        case 0x7f:

          std::cout << "Case 0x7f (" << std::dec << 0x7f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 4,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
                                         _mm_srli_epi32(byte_stream, 7)),
                            mask_7));
          byte_stream = _mm_srli_epi32(byte_stream_2, 3);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
                           _mm_and_si128(byte_stream, mask_7));
          byte_stream = _mm_srli_epi32(byte_stream, 7);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
                           _mm_and_si128(byte_stream, mask_7));
          in += 32;
          to += 36;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x80:

          std::cout << "Case 0x80 (" << std::dec << 0x80 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x81:

          std::cout << "Case 0x81 (" << std::dec << 0x81 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x82:

          std::cout << "Case 0x82 (" << std::dec << 0x82 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x83:

          std::cout << "Case 0x83 (" << std::dec << 0x83 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x84:

          std::cout << "Case 0x84 (" << std::dec << 0x84 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x85:

          std::cout << "Case 0x85 (" << std::dec << 0x85 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x86:

          std::cout << "Case 0x86 (" << std::dec << 0x86 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x87:

          std::cout << "Case 0x87 (" << std::dec << 0x87 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x88:

          std::cout << "Case 0x88 (" << std::dec << 0x88 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x89:

          std::cout << "Case 0x89 (" << std::dec << 0x89 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8a:

          std::cout << "Case 0x8a (" << std::dec << 0x8a << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8b:

          std::cout << "Case 0x8b (" << std::dec << 0x8b << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8c:

          std::cout << "Case 0x8c (" << std::dec << 0x8c << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8d:

          std::cout << "Case 0x8d (" << std::dec << 0x8d << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8e:

          std::cout << "Case 0x8e (" << std::dec << 0x8e << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;
        case 0x8f:

          std::cout << "Case 0x8f (" << std::dec << 0x8f << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu8_epi32(tmp2));
          tmp = _mm_castps_si128(
              _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_cvtepu8_epi32(tmp));
          tmp2 = _mm_castps_si128(
              _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_cvtepu8_epi32(tmp2));
          in += 16;
          to += 16;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0x90:

          std::cout << "Case 0x90 (" << std::dec << 0x90 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x91:

          std::cout << "Case 0x91 (" << std::dec << 0x91 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x92:

          std::cout << "Case 0x92 (" << std::dec << 0x92 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x93:

          std::cout << "Case 0x93 (" << std::dec << 0x93 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x94:

          std::cout << "Case 0x94 (" << std::dec << 0x94 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x95:

          std::cout << "Case 0x95 (" << std::dec << 0x95 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x96:

          std::cout << "Case 0x96 (" << std::dec << 0x96 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x97:

          std::cout << "Case 0x97 (" << std::dec << 0x97 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x98:

          std::cout << "Case 0x98 (" << std::dec << 0x98 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x99:

          std::cout << "Case 0x99 (" << std::dec << 0x99 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9a:

          std::cout << "Case 0x9a (" << std::dec << 0x9a << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9b:

          std::cout << "Case 0x9b (" << std::dec << 0x9b << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9c:

          std::cout << "Case 0x9c (" << std::dec << 0x9c << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9d:

          std::cout << "Case 0x9d (" << std::dec << 0x9d << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9e:

          std::cout << "Case 0x9e (" << std::dec << 0x9e << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;
        case 0x9f:

          std::cout << "Case 0x9f (" << std::dec << 0x9f << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 3,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
                                         _mm_srli_epi32(byte_stream, 9)),
                            mask_9));
          byte_stream = _mm_srli_epi32(byte_stream_2, 4);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
                           _mm_and_si128(byte_stream, mask_9));
          byte_stream = _mm_srli_epi32(byte_stream, 9);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
                           _mm_and_si128(byte_stream, mask_9));
          in += 32;
          to += 28;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xa0:

          std::cout << "Case 0xa0 (" << std::dec << 0xa0 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa1:

          std::cout << "Case 0xa1 (" << std::dec << 0xa1 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa2:

          std::cout << "Case 0xa2 (" << std::dec << 0xa2 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa3:

          std::cout << "Case 0xa3 (" << std::dec << 0xa3 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa4:

          std::cout << "Case 0xa4 (" << std::dec << 0xa4 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa5:

          std::cout << "Case 0xa5 (" << std::dec << 0xa5 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa6:

          std::cout << "Case 0xa6 (" << std::dec << 0xa6 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa7:

          std::cout << "Case 0xa7 (" << std::dec << 0xa7 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa8:

          std::cout << "Case 0xa8 (" << std::dec << 0xa8 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xa9:

          std::cout << "Case 0xa9 (" << std::dec << 0xa9 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xaa:

          std::cout << "Case 0xaa (" << std::dec << 0xaa << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xab:

          std::cout << "Case 0xab (" << std::dec << 0xab << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xac:

          std::cout << "Case 0xac (" << std::dec << 0xac << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xad:

          std::cout << "Case 0xad (" << std::dec << 0xad << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xae:

          std::cout << "Case 0xae (" << std::dec << 0xae << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;
        case 0xaf:

          std::cout << "Case 0xaf (" << std::dec << 0xaf << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_10));
          byte_stream = _mm_srli_epi64(byte_stream, 10);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
                           _mm_and_si128(byte_stream, mask_10));
          in += 16;
          to += 12;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xb0:

          std::cout << "Case 0xb0 (" << std::dec << 0xb0 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb1:

          std::cout << "Case 0xb1 (" << std::dec << 0xb1 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb2:

          std::cout << "Case 0xb2 (" << std::dec << 0xb2 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb3:

          std::cout << "Case 0xb3 (" << std::dec << 0xb3 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb4:

          std::cout << "Case 0xb4 (" << std::dec << 0xb4 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb5:

          std::cout << "Case 0xb5 (" << std::dec << 0xb5 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb6:

          std::cout << "Case 0xb6 (" << std::dec << 0xb6 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb7:

          std::cout << "Case 0xb7 (" << std::dec << 0xb7 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb8:

          std::cout << "Case 0xb8 (" << std::dec << 0xb8 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xb9:

          std::cout << "Case 0xb9 (" << std::dec << 0xb9 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xba:

          std::cout << "Case 0xba (" << std::dec << 0xba << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xbb:

          std::cout << "Case 0xbb (" << std::dec << 0xbb << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xbc:

          std::cout << "Case 0xbc (" << std::dec << 0xbc << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xbd:

          std::cout << "Case 0xbd (" << std::dec << 0xbd << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xbe:

          std::cout << "Case 0xbe (" << std::dec << 0xbe << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;
        case 0xbf:

          std::cout << "Case 0xbf (" << std::dec << 0xbf << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
                                         _mm_srli_epi32(byte_stream, 12)),
                            mask_12));
          byte_stream = _mm_srli_epi32(byte_stream_2, 8);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
                           _mm_and_si128(byte_stream, mask_12));
          byte_stream = _mm_srli_epi32(byte_stream, 12);
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
                           _mm_and_si128(byte_stream, mask_12));
          in += 32;
          to += 20;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xc0:

          std::cout << "Case 0xc0 (" << std::dec << 0xc0 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc1:

          std::cout << "Case 0xc1 (" << std::dec << 0xc1 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc2:

          std::cout << "Case 0xc2 (" << std::dec << 0xc2 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc3:

          std::cout << "Case 0xc3 (" << std::dec << 0xc3 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc4:

          std::cout << "Case 0xc4 (" << std::dec << 0xc4 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc5:

          std::cout << "Case 0xc5 (" << std::dec << 0xc5 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc6:

          std::cout << "Case 0xc6 (" << std::dec << 0xc6 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc7:

          std::cout << "Case 0xc7 (" << std::dec << 0xc7 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc8:

          std::cout << "Case 0xc8 (" << std::dec << 0xc8 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xc9:

          std::cout << "Case 0xc9 (" << std::dec << 0xc9 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xca:

          std::cout << "Case 0xca (" << std::dec << 0xca << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xcb:

          std::cout << "Case 0xcb (" << std::dec << 0xcb << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xcc:

          std::cout << "Case 0xcc (" << std::dec << 0xcc << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xcd:

          std::cout << "Case 0xcd (" << std::dec << 0xcd << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xce:

          std::cout << "Case 0xce (" << std::dec << 0xce << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;
        case 0xcf:

          std::cout << "Case 0xcf (" << std::dec << 0xcf << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_cvtepu16_epi32(tmp));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
                           _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
                               _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
          in += 16;
          to += 8;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xd0:

          std::cout << "Case 0xd0 (" << std::dec << 0xd0 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd1:

          std::cout << "Case 0xd1 (" << std::dec << 0xd1 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd2:

          std::cout << "Case 0xd2 (" << std::dec << 0xd2 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd3:

          std::cout << "Case 0xd3 (" << std::dec << 0xd3 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd4:

          std::cout << "Case 0xd4 (" << std::dec << 0xd4 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd5:

          std::cout << "Case 0xd5 (" << std::dec << 0xd5 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd6:

          std::cout << "Case 0xd6 (" << std::dec << 0xd6 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd7:

          std::cout << "Case 0xd7 (" << std::dec << 0xd7 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd8:

          std::cout << "Case 0xd8 (" << std::dec << 0xd8 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xd9:

          std::cout << "Case 0xd9 (" << std::dec << 0xd9 << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xda:

          std::cout << "Case 0xda (" << std::dec << 0xda << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xdb:

          std::cout << "Case 0xdb (" << std::dec << 0xdb << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xdc:

          std::cout << "Case 0xdc (" << std::dec << 0xdc << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xdd:

          std::cout << "Case 0xdd (" << std::dec << 0xdd << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xde:

          std::cout << "Case 0xde (" << std::dec << 0xde << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;
        case 0xdf:

          std::cout << "Case 0xdf (" << std::dec << 0xdf << ")" << std::endl;

          byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
                           _mm_and_si128(byte_stream, mask_21));
          byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 1,
              _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
                                         _mm_srli_epi32(byte_stream, 21)),
                            mask_21));
          _mm_storeu_si128(
              reinterpret_cast<__m128i *>(to) + 2,
              _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
          in += 32;
          to += 12;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xe0:

          std::cout << "Case 0xe0 (" << std::dec << 0xe0 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe1:

          std::cout << "Case 0xe1 (" << std::dec << 0xe1 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe2:

          std::cout << "Case 0xe2 (" << std::dec << 0xe2 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe3:

          std::cout << "Case 0xe3 (" << std::dec << 0xe3 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe4:

          std::cout << "Case 0xe4 (" << std::dec << 0xe4 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe5:

          std::cout << "Case 0xe5 (" << std::dec << 0xe5 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe6:

          std::cout << "Case 0xe6 (" << std::dec << 0xe6 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe7:

          std::cout << "Case 0xe7 (" << std::dec << 0xe7 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe8:

          std::cout << "Case 0xe8 (" << std::dec << 0xe8 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xe9:

          std::cout << "Case 0xe9 (" << std::dec << 0xe9 << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xea:

          std::cout << "Case 0xea (" << std::dec << 0xea << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xeb:

          std::cout << "Case 0xeb (" << std::dec << 0xeb << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xec:

          std::cout << "Case 0xec (" << std::dec << 0xec << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xed:

          std::cout << "Case 0xed (" << std::dec << 0xed << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xee:

          std::cout << "Case 0xee (" << std::dec << 0xee << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;
        case 0xef:

          std::cout << "Case 0xef (" << std::dec << 0xef << ")" << std::endl;

          tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
          _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
          in += 16;
          to += 4;

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
        case 0xf0:

          std::cout << "Case 0xf0 (" << std::dec << 0xf0 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf1:

          std::cout << "Case 0xf1 (" << std::dec << 0xf1 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf2:

          std::cout << "Case 0xf2 (" << std::dec << 0xf2 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf3:

          std::cout << "Case 0xf3 (" << std::dec << 0xf3 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf4:

          std::cout << "Case 0xf4 (" << std::dec << 0xf4 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf5:

          std::cout << "Case 0xf5 (" << std::dec << 0xf5 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf6:

          std::cout << "Case 0xf6 (" << std::dec << 0xf6 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf7:

          std::cout << "Case 0xf7 (" << std::dec << 0xf7 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf8:

          std::cout << "Case 0xf8 (" << std::dec << 0xf8 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xf9:

          std::cout << "Case 0xf9 (" << std::dec << 0xf9 << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xfa:

          std::cout << "Case 0xfa (" << std::dec << 0xfa << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xfb:

          std::cout << "Case 0xfb (" << std::dec << 0xfb << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xfc:

          std::cout << "Case 0xfc (" << std::dec << 0xfc << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xfd:

          std::cout << "Case 0xfd (" << std::dec << 0xfd << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xfe:

          std::cout << "Case 0xfe (" << std::dec << 0xfe << ")" << std::endl;

          in++; // LCOV_EXCL_LINE
        case 0xff:

          std::cout << "Case 0xff (" << std::dec << 0xff << ")" << std::endl;

          in++;  // LCOV_EXCL_LINE

          std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << "; *to = " << ((std::uint64_t)to) << "; to offset = " << ((std::uint64_t)(to - destination_base)) << std::endl;
          return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) }; // LCOV_EXCL_LINE
        }
        // std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << std::endl;
        // for(std::size_t i = 0 ; i < destination_integers; i++) {
        //   std::cout << "\t" << to[i] << std::endl;
        // }
      }
      std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << std::endl;
      return { (size_t) (in - ((uint8_t *)source) ) , (size_t) (((uint8_t *)source) + len - 1 - keys) ,  (size_t) (to - destination_base) };
    }
    // void decode(integer *to, size_t destination_integers, const void *source,
    //             size_t len)
    // {
    //   __m128i byte_stream, byte_stream_2, tmp, tmp2, mask_21, mask_12, mask_10,
    //       mask_9, mask_7, mask_6, mask_5, mask_4, mask_3, mask_2, mask_1;
    //   uint8_t *in = (uint8_t *)source;
    //   uint8_t *keys = ((uint8_t *)source) + len - 1;

    //   mask_21 = _mm_loadu_si128((__m128i *)static_mask_21);
    //   mask_12 = _mm_loadu_si128((__m128i *)static_mask_12);
    //   mask_10 = _mm_loadu_si128((__m128i *)static_mask_10);
    //   mask_9 = _mm_loadu_si128((__m128i *)static_mask_9);
    //   mask_7 = _mm_loadu_si128((__m128i *)static_mask_7);
    //   mask_6 = _mm_loadu_si128((__m128i *)static_mask_6);
    //   mask_5 = _mm_loadu_si128((__m128i *)static_mask_5);
    //   mask_4 = _mm_loadu_si128((__m128i *)static_mask_4);
    //   mask_3 = _mm_loadu_si128((__m128i *)static_mask_3);
    //   mask_2 = _mm_loadu_si128((__m128i *)static_mask_2);
    //   mask_1 = _mm_loadu_si128((__m128i *)static_mask_1);


    //   while (in <= keys) // <= because there can be a boundary case where the
    //                      // final key is 255*0 bit integers
    //   {
    //     std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << std::endl;
    //     switch (*keys--)
    //     {
    //     case 0x00:

    //       std::cout << "Case 0x00 (" << std::dec << 0x00 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x01:

    //       std::cout << "Case 0x01 (" << std::dec << 0x01 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x02:

    //       std::cout << "Case 0x02 (" << std::dec << 0x02 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x03:

    //       std::cout << "Case 0x03 (" << std::dec << 0x03 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x04:

    //       std::cout << "Case 0x04 (" << std::dec << 0x04 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x05:

    //       std::cout << "Case 0x05 (" << std::dec << 0x05 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x06:

    //       std::cout << "Case 0x06 (" << std::dec << 0x06 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x07:

    //       std::cout << "Case 0x07 (" << std::dec << 0x07 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x08:

    //       std::cout << "Case 0x08 (" << std::dec << 0x08 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x09:

    //       std::cout << "Case 0x09 (" << std::dec << 0x09 << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0a:

    //       std::cout << "Case 0x0a (" << std::dec << 0x0a << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0b:

    //       std::cout << "Case 0x0b (" << std::dec << 0x0b << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0c:

    //       std::cout << "Case 0x0c (" << std::dec << 0x0c << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0d:

    //       std::cout << "Case 0x0d (" << std::dec << 0x0d << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0e:

    //       std::cout << "Case 0x0e (" << std::dec << 0x0e << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //     case 0x0f:

    //       std::cout << "Case 0x0f (" << std::dec << 0x0f << ")" << std::endl;

    //       tmp = _mm_loadu_si128((__m128i *)static_mask_1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 32, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 33, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 34, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 35, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 36, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 37, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 38, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 39, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 40, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 41, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 42, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 43, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 44, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 45, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 46, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 47, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 48, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 49, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 50, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 51, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 52, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 53, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 54, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 55, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 56, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 57, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 58, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 59, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 60, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 61, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 62, tmp);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 63, tmp);
    //       to += 256;
    //       break;
    //     case 0x10:

    //       std::cout << "Case 0x10 (" << std::dec << 0x10 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x11:

    //       std::cout << "Case 0x11 (" << std::dec << 0x11 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x12:

    //       std::cout << "Case 0x12 (" << std::dec << 0x12 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x13:

    //       std::cout << "Case 0x13 (" << std::dec << 0x13 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x14:

    //       std::cout << "Case 0x14 (" << std::dec << 0x14 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x15:

    //       std::cout << "Case 0x15 (" << std::dec << 0x15 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x16:

    //       std::cout << "Case 0x16 (" << std::dec << 0x16 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x17:

    //       std::cout << "Case 0x17 (" << std::dec << 0x17 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x18:

    //       std::cout << "Case 0x18 (" << std::dec << 0x18 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x19:

    //       std::cout << "Case 0x19 (" << std::dec << 0x19 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1a:

    //       std::cout << "Case 0x1a (" << std::dec << 0x1a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1b:

    //       std::cout << "Case 0x1b (" << std::dec << 0x1b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1c:

    //       std::cout << "Case 0x1c (" << std::dec << 0x1c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1d:

    //       std::cout << "Case 0x1d (" << std::dec << 0x1d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1e:

    //       std::cout << "Case 0x1e (" << std::dec << 0x1e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //     case 0x1f:

    //       std::cout << "Case 0x1f (" << std::dec << 0x1f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 16,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 17,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 18,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 19,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 20,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 21,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 22,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 23,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 24,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 25,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 26,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 27,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 28,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 29,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 30,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       byte_stream = _mm_srli_epi64(byte_stream, 1);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 31,
    //                        _mm_and_si128(byte_stream, mask_1));
    //       in += 16;
    //       to += 128;
    //       break;
    //     case 0x20:

    //       std::cout << "Case 0x20 (" << std::dec << 0x20 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x21:

    //       std::cout << "Case 0x21 (" << std::dec << 0x21 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x22:

    //       std::cout << "Case 0x22 (" << std::dec << 0x22 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x23:

    //       std::cout << "Case 0x23 (" << std::dec << 0x23 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x24:

    //       std::cout << "Case 0x24 (" << std::dec << 0x24 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x25:

    //       std::cout << "Case 0x25 (" << std::dec << 0x25 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x26:

    //       std::cout << "Case 0x26 (" << std::dec << 0x26 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x27:

    //       std::cout << "Case 0x27 (" << std::dec << 0x27 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x28:

    //       std::cout << "Case 0x28 (" << std::dec << 0x28 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x29:

    //       std::cout << "Case 0x29 (" << std::dec << 0x29 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2a:

    //       std::cout << "Case 0x2a (" << std::dec << 0x2a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2b:

    //       std::cout << "Case 0x2b (" << std::dec << 0x2b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2c:

    //       std::cout << "Case 0x2c (" << std::dec << 0x2c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2d:

    //       std::cout << "Case 0x2d (" << std::dec << 0x2d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2e:

    //       std::cout << "Case 0x2e (" << std::dec << 0x2e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //     case 0x2f:

    //       std::cout << "Case 0x2f (" << std::dec << 0x2f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 10,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 11,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 12,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 13,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 14,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       byte_stream = _mm_srli_epi64(byte_stream, 2);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 15,
    //                        _mm_and_si128(byte_stream, mask_2));
    //       in += 16;
    //       to += 64;
    //       break;
    //     case 0x30:

    //       std::cout << "Case 0x30 (" << std::dec << 0x30 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x31:

    //       std::cout << "Case 0x31 (" << std::dec << 0x31 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x32:

    //       std::cout << "Case 0x32 (" << std::dec << 0x32 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x33:

    //       std::cout << "Case 0x33 (" << std::dec << 0x33 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x34:

    //       std::cout << "Case 0x34 (" << std::dec << 0x34 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x35:

    //       std::cout << "Case 0x35 (" << std::dec << 0x35 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x36:

    //       std::cout << "Case 0x36 (" << std::dec << 0x36 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x37:

    //       std::cout << "Case 0x37 (" << std::dec << 0x37 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x38:

    //       std::cout << "Case 0x38 (" << std::dec << 0x38 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x39:

    //       std::cout << "Case 0x39 (" << std::dec << 0x39 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3a:

    //       std::cout << "Case 0x3a (" << std::dec << 0x3a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3b:

    //       std::cout << "Case 0x3b (" << std::dec << 0x3b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3c:

    //       std::cout << "Case 0x3c (" << std::dec << 0x3c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3d:

    //       std::cout << "Case 0x3d (" << std::dec << 0x3d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3e:

    //       std::cout << "Case 0x3e (" << std::dec << 0x3e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //     case 0x3f:

    //       std::cout << "Case 0x3f (" << std::dec << 0x3f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       byte_stream = _mm_srli_epi64(byte_stream, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 9,
    //                        _mm_and_si128(byte_stream, mask_3));
    //       in += 16;
    //       to += 40;
    //       break;
    //     case 0x40:

    //       std::cout << "Case 0x40 (" << std::dec << 0x40 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x41:

    //       std::cout << "Case 0x41 (" << std::dec << 0x41 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x42:

    //       std::cout << "Case 0x42 (" << std::dec << 0x42 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x43:

    //       std::cout << "Case 0x43 (" << std::dec << 0x43 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x44:

    //       std::cout << "Case 0x44 (" << std::dec << 0x44 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x45:

    //       std::cout << "Case 0x45 (" << std::dec << 0x45 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x46:

    //       std::cout << "Case 0x46 (" << std::dec << 0x46 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x47:

    //       std::cout << "Case 0x47 (" << std::dec << 0x47 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x48:

    //       std::cout << "Case 0x48 (" << std::dec << 0x48 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x49:

    //       std::cout << "Case 0x49 (" << std::dec << 0x49 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4a:

    //       std::cout << "Case 0x4a (" << std::dec << 0x4a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4b:

    //       std::cout << "Case 0x4b (" << std::dec << 0x4b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4c:

    //       std::cout << "Case 0x4c (" << std::dec << 0x4c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4d:

    //       std::cout << "Case 0x4d (" << std::dec << 0x4d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4e:

    //       std::cout << "Case 0x4e (" << std::dec << 0x4e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //     case 0x4f:

    //       std::cout << "Case 0x4f (" << std::dec << 0x4f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       byte_stream = _mm_srli_epi64(byte_stream, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_4));
    //       in += 16;
    //       to += 32;
    //       break;
    //     case 0x50:

    //       std::cout << "Case 0x50 (" << std::dec << 0x50 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x51:

    //       std::cout << "Case 0x51 (" << std::dec << 0x51 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x52:

    //       std::cout << "Case 0x52 (" << std::dec << 0x52 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x53:

    //       std::cout << "Case 0x53 (" << std::dec << 0x53 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x54:

    //       std::cout << "Case 0x54 (" << std::dec << 0x54 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x55:

    //       std::cout << "Case 0x55 (" << std::dec << 0x55 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x56:

    //       std::cout << "Case 0x56 (" << std::dec << 0x56 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x57:

    //       std::cout << "Case 0x57 (" << std::dec << 0x57 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x58:

    //       std::cout << "Case 0x58 (" << std::dec << 0x58 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x59:

    //       std::cout << "Case 0x59 (" << std::dec << 0x59 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5a:

    //       std::cout << "Case 0x5a (" << std::dec << 0x5a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5b:

    //       std::cout << "Case 0x5b (" << std::dec << 0x5b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5c:

    //       std::cout << "Case 0x5c (" << std::dec << 0x5c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5d:

    //       std::cout << "Case 0x5d (" << std::dec << 0x5d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5e:

    //       std::cout << "Case 0x5e (" << std::dec << 0x5e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //     case 0x5f:

    //       std::cout << "Case 0x5f (" << std::dec << 0x5f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       byte_stream = _mm_srli_epi64(byte_stream, 5);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_5));
    //       in += 16;
    //       to += 24;
    //       break;
    //     case 0x60:

    //       std::cout << "Case 0x60 (" << std::dec << 0x60 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x61:

    //       std::cout << "Case 0x61 (" << std::dec << 0x61 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x62:

    //       std::cout << "Case 0x62 (" << std::dec << 0x62 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x63:

    //       std::cout << "Case 0x63 (" << std::dec << 0x63 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x64:

    //       std::cout << "Case 0x64 (" << std::dec << 0x64 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x65:

    //       std::cout << "Case 0x65 (" << std::dec << 0x65 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x66:

    //       std::cout << "Case 0x66 (" << std::dec << 0x66 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x67:

    //       std::cout << "Case 0x67 (" << std::dec << 0x67 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x68:

    //       std::cout << "Case 0x68 (" << std::dec << 0x68 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x69:

    //       std::cout << "Case 0x69 (" << std::dec << 0x69 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6a:

    //       std::cout << "Case 0x6a (" << std::dec << 0x6a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6b:

    //       std::cout << "Case 0x6b (" << std::dec << 0x6b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6c:

    //       std::cout << "Case 0x6c (" << std::dec << 0x6c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6d:

    //       std::cout << "Case 0x6d (" << std::dec << 0x6d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6e:

    //       std::cout << "Case 0x6e (" << std::dec << 0x6e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //     case 0x6f:

    //       std::cout << "Case 0x6f (" << std::dec << 0x6f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       byte_stream = _mm_srli_epi64(byte_stream, 6);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_6));
    //       in += 16;
    //       to += 20;
    //       break;
    //     case 0x70:

    //       std::cout << "Case 0x70 (" << std::dec << 0x70 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x71:

    //       std::cout << "Case 0x71 (" << std::dec << 0x71 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x72:

    //       std::cout << "Case 0x72 (" << std::dec << 0x72 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x73:

    //       std::cout << "Case 0x73 (" << std::dec << 0x73 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x74:

    //       std::cout << "Case 0x74 (" << std::dec << 0x74 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x75:

    //       std::cout << "Case 0x75 (" << std::dec << 0x75 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x76:

    //       std::cout << "Case 0x76 (" << std::dec << 0x76 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x77:

    //       std::cout << "Case 0x77 (" << std::dec << 0x77 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x78:

    //       std::cout << "Case 0x78 (" << std::dec << 0x78 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x79:

    //       std::cout << "Case 0x79 (" << std::dec << 0x79 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7a:

    //       std::cout << "Case 0x7a (" << std::dec << 0x7a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7b:

    //       std::cout << "Case 0x7b (" << std::dec << 0x7b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7c:

    //       std::cout << "Case 0x7c (" << std::dec << 0x7c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7d:

    //       std::cout << "Case 0x7d (" << std::dec << 0x7d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7e:

    //       std::cout << "Case 0x7e (" << std::dec << 0x7e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //     case 0x7f:

    //       std::cout << "Case 0x7f (" << std::dec << 0x7f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 4,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 4),
    //                                      _mm_srli_epi32(byte_stream, 7)),
    //                         mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 3);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 7,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       byte_stream = _mm_srli_epi32(byte_stream, 7);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 8,
    //                        _mm_and_si128(byte_stream, mask_7));
    //       in += 32;
    //       to += 36;
    //       break;
    //     case 0x80:

    //       std::cout << "Case 0x80 (" << std::dec << 0x80 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x81:

    //       std::cout << "Case 0x81 (" << std::dec << 0x81 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x82:

    //       std::cout << "Case 0x82 (" << std::dec << 0x82 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x83:

    //       std::cout << "Case 0x83 (" << std::dec << 0x83 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x84:

    //       std::cout << "Case 0x84 (" << std::dec << 0x84 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x85:

    //       std::cout << "Case 0x85 (" << std::dec << 0x85 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x86:

    //       std::cout << "Case 0x86 (" << std::dec << 0x86 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x87:

    //       std::cout << "Case 0x87 (" << std::dec << 0x87 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x88:

    //       std::cout << "Case 0x88 (" << std::dec << 0x88 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x89:

    //       std::cout << "Case 0x89 (" << std::dec << 0x89 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8a:

    //       std::cout << "Case 0x8a (" << std::dec << 0x8a << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8b:

    //       std::cout << "Case 0x8b (" << std::dec << 0x8b << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8c:

    //       std::cout << "Case 0x8c (" << std::dec << 0x8c << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8d:

    //       std::cout << "Case 0x8d (" << std::dec << 0x8d << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8e:

    //       std::cout << "Case 0x8e (" << std::dec << 0x8e << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //     case 0x8f:

    //       std::cout << "Case 0x8f (" << std::dec << 0x8f << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       tmp = _mm_castps_si128(
    //           _mm_movehl_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_cvtepu8_epi32(tmp));
    //       tmp2 = _mm_castps_si128(
    //           _mm_shuffle_ps(_mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp), 0x01));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_cvtepu8_epi32(tmp2));
    //       in += 16;
    //       to += 16;
    //       break;
    //     case 0x90:

    //       std::cout << "Case 0x90 (" << std::dec << 0x90 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x91:

    //       std::cout << "Case 0x91 (" << std::dec << 0x91 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x92:

    //       std::cout << "Case 0x92 (" << std::dec << 0x92 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x93:

    //       std::cout << "Case 0x93 (" << std::dec << 0x93 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x94:

    //       std::cout << "Case 0x94 (" << std::dec << 0x94 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x95:

    //       std::cout << "Case 0x95 (" << std::dec << 0x95 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x96:

    //       std::cout << "Case 0x96 (" << std::dec << 0x96 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x97:

    //       std::cout << "Case 0x97 (" << std::dec << 0x97 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x98:

    //       std::cout << "Case 0x98 (" << std::dec << 0x98 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x99:

    //       std::cout << "Case 0x99 (" << std::dec << 0x99 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9a:

    //       std::cout << "Case 0x9a (" << std::dec << 0x9a << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9b:

    //       std::cout << "Case 0x9b (" << std::dec << 0x9b << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9c:

    //       std::cout << "Case 0x9c (" << std::dec << 0x9c << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9d:

    //       std::cout << "Case 0x9d (" << std::dec << 0x9d << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9e:

    //       std::cout << "Case 0x9e (" << std::dec << 0x9e << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //     case 0x9f:

    //       std::cout << "Case 0x9f (" << std::dec << 0x9f << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 3,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 5),
    //                                      _mm_srli_epi32(byte_stream, 9)),
    //                         mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 4);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 5,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       byte_stream = _mm_srli_epi32(byte_stream, 9);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 6,
    //                        _mm_and_si128(byte_stream, mask_9));
    //       in += 32;
    //       to += 28;
    //       break;
    //     case 0xa0:

    //       std::cout << "Case 0xa0 (" << std::dec << 0xa0 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa1:

    //       std::cout << "Case 0xa1 (" << std::dec << 0xa1 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa2:

    //       std::cout << "Case 0xa2 (" << std::dec << 0xa2 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa3:

    //       std::cout << "Case 0xa3 (" << std::dec << 0xa3 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa4:

    //       std::cout << "Case 0xa4 (" << std::dec << 0xa4 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa5:

    //       std::cout << "Case 0xa5 (" << std::dec << 0xa5 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa6:

    //       std::cout << "Case 0xa6 (" << std::dec << 0xa6 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa7:

    //       std::cout << "Case 0xa7 (" << std::dec << 0xa7 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa8:

    //       std::cout << "Case 0xa8 (" << std::dec << 0xa8 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xa9:

    //       std::cout << "Case 0xa9 (" << std::dec << 0xa9 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xaa:

    //       std::cout << "Case 0xaa (" << std::dec << 0xaa << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xab:

    //       std::cout << "Case 0xab (" << std::dec << 0xab << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xac:

    //       std::cout << "Case 0xac (" << std::dec << 0xac << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xad:

    //       std::cout << "Case 0xad (" << std::dec << 0xad << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xae:

    //       std::cout << "Case 0xae (" << std::dec << 0xae << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //     case 0xaf:

    //       std::cout << "Case 0xaf (" << std::dec << 0xaf << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       byte_stream = _mm_srli_epi64(byte_stream, 10);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 2,
    //                        _mm_and_si128(byte_stream, mask_10));
    //       in += 16;
    //       to += 12;
    //       break;
    //     case 0xb0:

    //       std::cout << "Case 0xb0 (" << std::dec << 0xb0 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb1:

    //       std::cout << "Case 0xb1 (" << std::dec << 0xb1 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb2:

    //       std::cout << "Case 0xb2 (" << std::dec << 0xb2 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb3:

    //       std::cout << "Case 0xb3 (" << std::dec << 0xb3 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb4:

    //       std::cout << "Case 0xb4 (" << std::dec << 0xb4 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb5:

    //       std::cout << "Case 0xb5 (" << std::dec << 0xb5 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb6:

    //       std::cout << "Case 0xb6 (" << std::dec << 0xb6 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb7:

    //       std::cout << "Case 0xb7 (" << std::dec << 0xb7 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb8:

    //       std::cout << "Case 0xb8 (" << std::dec << 0xb8 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xb9:

    //       std::cout << "Case 0xb9 (" << std::dec << 0xb9 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xba:

    //       std::cout << "Case 0xba (" << std::dec << 0xba << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xbb:

    //       std::cout << "Case 0xbb (" << std::dec << 0xbb << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xbc:

    //       std::cout << "Case 0xbc (" << std::dec << 0xbc << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xbd:

    //       std::cout << "Case 0xbd (" << std::dec << 0xbd << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xbe:

    //       std::cout << "Case 0xbe (" << std::dec << 0xbe << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //     case 0xbf:

    //       std::cout << "Case 0xbf (" << std::dec << 0xbf << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 8),
    //                                      _mm_srli_epi32(byte_stream, 12)),
    //                         mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream_2, 8);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 3,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       byte_stream = _mm_srli_epi32(byte_stream, 12);
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 4,
    //                        _mm_and_si128(byte_stream, mask_12));
    //       in += 32;
    //       to += 20;
    //       break;
    //     case 0xc0:

    //       std::cout << "Case 0xc0 (" << std::dec << 0xc0 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc1:

    //       std::cout << "Case 0xc1 (" << std::dec << 0xc1 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc2:

    //       std::cout << "Case 0xc2 (" << std::dec << 0xc2 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc3:

    //       std::cout << "Case 0xc3 (" << std::dec << 0xc3 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc4:

    //       std::cout << "Case 0xc4 (" << std::dec << 0xc4 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc5:

    //       std::cout << "Case 0xc5 (" << std::dec << 0xc5 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc6:

    //       std::cout << "Case 0xc6 (" << std::dec << 0xc6 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc7:

    //       std::cout << "Case 0xc7 (" << std::dec << 0xc7 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc8:

    //       std::cout << "Case 0xc8 (" << std::dec << 0xc8 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xc9:

    //       std::cout << "Case 0xc9 (" << std::dec << 0xc9 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xca:

    //       std::cout << "Case 0xca (" << std::dec << 0xca << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xcb:

    //       std::cout << "Case 0xcb (" << std::dec << 0xcb << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xcc:

    //       std::cout << "Case 0xcc (" << std::dec << 0xcc << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xcd:

    //       std::cout << "Case 0xcd (" << std::dec << 0xcd << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xce:

    //       std::cout << "Case 0xce (" << std::dec << 0xce << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //     case 0xcf:

    //       std::cout << "Case 0xcf (" << std::dec << 0xcf << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_cvtepu16_epi32(tmp));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to) + 1,
    //                        _mm_cvtepu16_epi32(_mm_castps_si128(_mm_movehl_ps(
    //                            _mm_castsi128_ps(tmp), _mm_castsi128_ps(tmp)))));
    //       in += 16;
    //       to += 8;
    //       break;
    //     case 0xd0:

    //       std::cout << "Case 0xd0 (" << std::dec << 0xd0 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd1:

    //       std::cout << "Case 0xd1 (" << std::dec << 0xd1 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd2:

    //       std::cout << "Case 0xd2 (" << std::dec << 0xd2 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd3:

    //       std::cout << "Case 0xd3 (" << std::dec << 0xd3 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd4:

    //       std::cout << "Case 0xd4 (" << std::dec << 0xd4 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd5:

    //       std::cout << "Case 0xd5 (" << std::dec << 0xd5 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd6:

    //       std::cout << "Case 0xd6 (" << std::dec << 0xd6 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd7:

    //       std::cout << "Case 0xd7 (" << std::dec << 0xd7 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd8:

    //       std::cout << "Case 0xd8 (" << std::dec << 0xd8 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xd9:

    //       std::cout << "Case 0xd9 (" << std::dec << 0xd9 << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xda:

    //       std::cout << "Case 0xda (" << std::dec << 0xda << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xdb:

    //       std::cout << "Case 0xdb (" << std::dec << 0xdb << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xdc:

    //       std::cout << "Case 0xdc (" << std::dec << 0xdc << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xdd:

    //       std::cout << "Case 0xdd (" << std::dec << 0xdd << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xde:

    //       std::cout << "Case 0xde (" << std::dec << 0xde << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //     case 0xdf:

    //       std::cout << "Case 0xdf (" << std::dec << 0xdf << ")" << std::endl;

    //       byte_stream = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to),
    //                        _mm_and_si128(byte_stream, mask_21));
    //       byte_stream_2 = _mm_loadu_si128(reinterpret_cast<__m128i *>(in) + 1);
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 1,
    //           _mm_and_si128(_mm_or_si128(_mm_slli_epi32(byte_stream_2, 11),
    //                                      _mm_srli_epi32(byte_stream, 21)),
    //                         mask_21));
    //       _mm_storeu_si128(
    //           reinterpret_cast<__m128i *>(to) + 2,
    //           _mm_and_si128(_mm_srli_epi32(byte_stream_2, 11), mask_21));
    //       in += 32;
    //       to += 12;
    //       break;
    //     case 0xe0:

    //       std::cout << "Case 0xe0 (" << std::dec << 0xe0 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe1:

    //       std::cout << "Case 0xe1 (" << std::dec << 0xe1 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe2:

    //       std::cout << "Case 0xe2 (" << std::dec << 0xe2 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe3:

    //       std::cout << "Case 0xe3 (" << std::dec << 0xe3 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe4:

    //       std::cout << "Case 0xe4 (" << std::dec << 0xe4 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe5:

    //       std::cout << "Case 0xe5 (" << std::dec << 0xe5 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe6:

    //       std::cout << "Case 0xe6 (" << std::dec << 0xe6 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe7:

    //       std::cout << "Case 0xe7 (" << std::dec << 0xe7 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe8:

    //       std::cout << "Case 0xe8 (" << std::dec << 0xe8 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xe9:

    //       std::cout << "Case 0xe9 (" << std::dec << 0xe9 << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xea:

    //       std::cout << "Case 0xea (" << std::dec << 0xea << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xeb:

    //       std::cout << "Case 0xeb (" << std::dec << 0xeb << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xec:

    //       std::cout << "Case 0xec (" << std::dec << 0xec << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xed:

    //       std::cout << "Case 0xed (" << std::dec << 0xed << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xee:

    //       std::cout << "Case 0xee (" << std::dec << 0xee << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //     case 0xef:

    //       std::cout << "Case 0xef (" << std::dec << 0xef << ")" << std::endl;

    //       tmp = _mm_loadu_si128(reinterpret_cast<__m128i *>(in));
    //       _mm_storeu_si128(reinterpret_cast<__m128i *>(to), tmp);
    //       in += 16;
    //       to += 4;
    //       break;
    //     case 0xf0:

    //       std::cout << "Case 0xf0 (" << std::dec << 0xf0 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf1:

    //       std::cout << "Case 0xf1 (" << std::dec << 0xf1 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf2:

    //       std::cout << "Case 0xf2 (" << std::dec << 0xf2 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf3:

    //       std::cout << "Case 0xf3 (" << std::dec << 0xf3 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf4:

    //       std::cout << "Case 0xf4 (" << std::dec << 0xf4 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf5:

    //       std::cout << "Case 0xf5 (" << std::dec << 0xf5 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf6:

    //       std::cout << "Case 0xf6 (" << std::dec << 0xf6 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf7:

    //       std::cout << "Case 0xf7 (" << std::dec << 0xf7 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf8:

    //       std::cout << "Case 0xf8 (" << std::dec << 0xf8 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xf9:

    //       std::cout << "Case 0xf9 (" << std::dec << 0xf9 << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xfa:

    //       std::cout << "Case 0xfa (" << std::dec << 0xfa << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xfb:

    //       std::cout << "Case 0xfb (" << std::dec << 0xfb << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xfc:

    //       std::cout << "Case 0xfc (" << std::dec << 0xfc << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xfd:

    //       std::cout << "Case 0xfd (" << std::dec << 0xfd << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xfe:

    //       std::cout << "Case 0xfe (" << std::dec << 0xfe << ")" << std::endl;

    //       in++; // LCOV_EXCL_LINE
    //     case 0xff:

    //       std::cout << "Case 0xff (" << std::dec << 0xff << ")" << std::endl;

    //       in++;  // LCOV_EXCL_LINE
    //       break; // LCOV_EXCL_LINE
    //     }
    //     // std::cout << "* *In = " << ((std::uint16_t)*in) << "; In = " << ((std::uint64_t)in) << "; *keys = " << ((std::uint16_t)*keys) << "; keys = " << ((std::uint64_t)keys) << std::endl;
    //     // for(std::size_t i = 0 ; i < destination_integers; i++) {
    //     //   std::cout << "\t" << to[i] << std::endl;
    //     // }
    //   }
    // }
  };
} // namespace QMX
