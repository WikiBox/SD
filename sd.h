/*
 * sd.h - Compressed String Dictionary with prefix encoding
 * Copyright (C) 2019 Anders Larsen <gislagard@gmail.com>
 *
 * SD is free software: you can redistribute it and/or modify it
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

#ifndef _SD_H_
#define _SD_H_

#include <string>
#include <vector>
#include <cstdint>
#include <linux/limits.h> // PATH_MAX and NAME_MAX
#include <fstream>

class SD
{
public:

    SD()
    {
        previous.reserve(PATH_MAX + NAME_MAX + 1); // Avoid reallocs == faster
        previous = "";
        count = 0;
    };

    int32_t get_count(void)
    {
        return count;
    };
    int32_t add(const std::string& s);
    int32_t locate(const std::string& target);
    std::string extract(const int32_t i);

    void serialize(std::ostream &os);
    void deserialize(std::istream &is);

private:
    int32_t count;
    size_t BUCKET_SIZE = 128;
    std::string previous;
    std::vector<std::string> buckets;
};

#endif // _SD_H_