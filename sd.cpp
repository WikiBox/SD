/*
 * sd.cpp - String Dictionary with prefix encoding
 * Copyright (C) 2019 Anders Larsen <gislagard@gmail.com>
 *
 * backbit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * backbit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sd.h"
#include <vector>
#include <string>
#include <fstream>

int32_t SD::add(const std::string& s)
{
    // assert(s.length() < 32768);
    // assert(s > previous);

    // Time to add a new bucket? Or the very first bucket?
    if ((count % BUCKET_SIZE) == 0)
    {
        // Try to shrink the previous bucket, if any
        if (count) buckets.back().shrink_to_fit();

        // Store first dictionary string in bucket uncompressed     
        buckets.push_back(s);
        buckets.back() += '\0';
    }
        // Store s compressed, based on common prefix from previous?
    else
    {
        int16_t lcp = 0; // Length Common Prefix

        while (s[lcp] == previous[lcp])
            lcp++;

        // Get a reference to the current bucket, for convenience
        std::string& current = buckets.back();

        // Store lcp as two bytes if lcp is 128-32767 bytes long
        if (lcp > 127)
        {
            // leftmost bit == 1
            current += (unsigned char) ((lcp & 127) | 0x80);
            current += (unsigned char) (lcp >> 7);
        }
            // Store lcp as one byte if lcp is 0-127 bytes long, leftmost bit == 0
        else
            current += (unsigned char) (lcp);

        current += s.substr(lcp);
        current += '\0'; // Zero terminated dictionary string
    }

    previous = s;
    count++;
    return count - 1;
}

std::string SD::extract(int32_t i)
{
    // assert(i < count);

    int16_t lcp; // Length Common Prefix
    auto it = buckets[i / BUCKET_SIZE].begin(); //  Integer division and ...
    std::string candidate = "";

    candidate.reserve(PATH_MAX + NAME_MAX + 1);

    // ... modulo to fast forward i among buckets
    i = i % BUCKET_SIZE;

    // Get first uncompressed candidate dictionary string from bucket
    while (*it != '\0')
    {
        candidate += *it;
        it++;
    }

    it++;

    // Decode bucket until correct string is candidate
    for (int16_t j = 1; j < (i + 1); j++)
    {
        lcp = *it;

        // Is lcp stored as two bytes? Leftmost bit == 1?
        if (lcp & 0x80)
        {
            it++;
            lcp = (lcp & 127) + (*it << 7); // Yes it is
        }

        // Keep only the common prefix
        candidate.resize(lcp);

        it++;

        // Get uncompressed part of dictionary string
        while (*it != '\0')
        {
            candidate += *it;
            it++;
        }

        it++;
    }

    return candidate;
}

int32_t SD::locate(const std::string& target)
{
    // assert(target.length() < 32768;  // Hard limit
    // assert(target.length() < (PATH_MAX + NAME_MAX + 1)); // Soft limit

    int32_t left = 0;
    int32_t middle = 0;
    int32_t right = buckets.size() - 1;
    std::string candidate;

    candidate.reserve(PATH_MAX + NAME_MAX + 1); // Avoid reallocs for speed

    // Binary search for the correct bucket
    while (left != right)
    {
        middle = (left + right) / 2 + ((left + right) % 2 != 0); // ceil x/2

        auto it = buckets[middle].begin();

        candidate = "";

        // Get first uncompressed candidate dictionary string from bucket
        while (*it != '\0')
        {
            candidate += *it;
            it++;
        }

        if (candidate > target)
            right = middle - 1;
        else
            left = middle;
    }

    if (candidate > target)
        middle--;

    // Linear search for target in the middle bucket
    auto it = buckets[middle].begin();
    const auto it_end = buckets[middle].end();
    int32_t candidate_index = middle * BUCKET_SIZE;

    candidate = "\0";

    // Get first uncompressed candidate dictionary string from bucket
    while (*it != '\0')
    {
        candidate += *it;
        it++;
    }

    it++;

    while (target > candidate && it != it_end)
    {
        int16_t lcp;
        candidate_index++;

        lcp = *it;

        // Is lcp stored as two bytes?
        if (lcp & 0x80)
        {
            it++;
            lcp = (lcp & 127) + (*it << 7); // Yes it is
        }

        // Truncate, keeping only the common prefix

        candidate.resize(lcp); // marginally faster
        // candidate.erase(lcp, std::string::npos);

        it++;

        // Get uncompressed part of dictionary string
        while (*it != '\0')
        {
            candidate += *it;
            it++;
        }

        it++;
    }

    if (target == candidate)
        return candidate_index; // Found!
    else
        return -1; // Not found!     
}

void SD::serialize(std::ostream &os)
{
    if (os.good())
    {
        size_t noOfBuckets = buckets.size();

        os.write(reinterpret_cast<const char *> (&noOfBuckets), sizeof (noOfBuckets));
        os.write(reinterpret_cast<const char *> (&count), sizeof (count));
        os.write(reinterpret_cast<const char *> (&BUCKET_SIZE), sizeof (BUCKET_SIZE));

        for (auto i = buckets.begin(); i != buckets.end(); i++)
        {
            std::string& bucket = *i;
            size_t sizeOfBucket = bucket.size();
            os.write(reinterpret_cast<const char *> (&sizeOfBucket), sizeof (sizeOfBucket));
            os.write(reinterpret_cast<const char *> (&bucket[0]), sizeOfBucket);
        }
    }
    if (os.fail())
        throw std::runtime_error("failed to write String Dictionary");
}

void SD::deserialize(std::istream &is)
{
    size_t noOfBuckets;

    buckets.clear();
    count = 0;

    if (is.good())
    {
        is.read(reinterpret_cast<char *> (&noOfBuckets), sizeof (noOfBuckets));
        is.read(reinterpret_cast<char *> (&count), sizeof (count));
        is.read(reinterpret_cast<char *> (&BUCKET_SIZE), sizeof (BUCKET_SIZE));

        for (int i = 0; i != noOfBuckets; i++)
        {
            size_t sizeOfBucket;
            std::string s;

            is.read(reinterpret_cast<char *> (&sizeOfBucket), sizeof (sizeOfBucket));
            s.resize(sizeOfBucket);
            is.read(reinterpret_cast<char *> (&s[0]), sizeOfBucket);
            buckets.push_back(s);
        }
    }
    if (is.fail())
        throw std::runtime_error("failed to read String Dictionary");
}