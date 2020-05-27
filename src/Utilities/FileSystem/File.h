// Copyright(c) 2019 - 2020, #Momo
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and /or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <filesystem>
#include <fstream>

#include "Utilities/STL/MxString.h"

namespace MxEngine
{
    using FilePath = std::filesystem::path;
    using FileStream = std::fstream;
    using FileSystemTime = std::filesystem::file_time_type;

    /*!
    File is an abstraction over std::fstream object and std::filesystem::path. 
    It caches data, searches files and used for reading/writing binat and text data.
    Note: consider using Script class for files which are frequently updated by user
    */
    class File
    {
        /*!
        fileStream object contains readed data from disk. Does NOT update automatically
        */
        FileStream fileStream;
        /*!
        file path associated with fileStream
        */
        FilePath filePath;
    public:
        /*!
        mode is used to specify how to treat opened file. Use binary OR ( | ) to compine mods
        */
        enum FileMode
        {
            READ = 0x1, // open for reading
            WRITE = 0x2, // open for writing
            BINARY = 0x4, // open as binary
        };
        using FileData = MxString;

        /*!
        creates empty file object (empty stream, empty path)
        */
        File() = default;
        /*!
        creates file object and opens fstream
        \param path path to a file
        \param mode how to treat file (see FileMode documentation)
        */
        File(const FilePath& path, FileMode mode = FileMode::READ);
        /*!
        creates file object and opens fstream
        \param path path to a file (absolute or relative to executable directory)
        \param mode how to treat file (see FileMode documentation)
        */
        File(const MxString& path, FileMode mode = FileMode::READ);
        File(File&) = default;
        File(File&&) = default;
        File& operator=(const File&) = default;
        File& operator=(File&&) = default;
        ~File() = default;

        /*!
        fileStream member getter
        \returns underlying std::fstream object
        */
        FileStream& GetStream();
        /*!
        checks if current file is open or not
        */
        bool IsOpen() const;
        /*!
        opens new file, old file associated with File is closed automatically
        \param path path to a file
        \param mode how to treat file (see FileMode documentation)
        */
        void Open(FilePath path, FileMode mode = FileMode::READ);
        /*!
        opens new file, old file associated with File is closed automatically
        \param path path to a file (absolute or relative to executable directory)
        \param mode how to treat file (see FileMode documentation)
        */
        void Open(const MxString& path, FileMode mode = FileMode::READ);
        /*
        closes currently opened std::fstream object (file)
        */
        void Close();
        /*!
        reads all data from file into string object. File must be opened for read and NOT binary
        \returns file contents as string. If file is not opened returns empty string
        */
        FileData ReadAllText();
        /*!
        gets current path associated with File object
        \returns path to a file
        */
        const FilePath& GetPath() const;

        /*!
        reads data from a file and treats it as T type
        \param value value to read to
        */
        template<typename T>
        File& operator>>(T& value);
        /*!
        writes data of type T to a file
        \param value value to write to
        */
        template<typename T>
        File& operator<<(T& value);

        /*!
        creates temporary File object and reads its data into  string.
        \param path path to a file
        \returns file contents as string. If file is not opened / not exists returns empty string
        */
        static FileData ReadAllText(const FilePath& path);
        /*!
        creates temporary File object and reads its data into  string.
        \param path path to a file (absolute or relative to executable directory)
        \returns file contents as string. If file is not opened / not exists returns empty string
        */
        static FileData ReadAllText(const MxString& path);
        /*!
        checks if file exists
        \param path path of file to search for
        \returns true if file exists, false either
        */
        static bool Exists(const FilePath& path);
        /*!
        gets file last modified time
        \param path path to a file to check for
        \returns platform-dependent time point of last file modification
        */
        static FileSystemTime LastModifiedTime(const FilePath& path);
    };

    template<typename T>
    inline File& File::operator>>(T& value)
    {
        this->fileStream >> value;
        return *this;
    }

    template<typename T>
    inline File& File::operator<<(T& value)
    {
        this->fileStream << value;
        return *this;
    }

    inline auto ToMxString(const FilePath& path)
    {
        return ToMxString(path.string());
    }
}