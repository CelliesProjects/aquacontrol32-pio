/*
MIT License

Copyright (c) 2025 Cellie https://github.com/CelliesProjects/aquacontrol32-pio/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "ScopedFile.h"

ScopedFile::ScopedFile(const char *filename, FileMode mode, uint8_t sdPin, uint32_t frequency)
{
    open(filename, mode, sdPin, frequency);
}

ScopedFile::ScopedFile(const String &filename, FileMode mode, uint8_t sdPin, uint32_t frequency)
{
    open(filename.c_str(), mode, sdPin, frequency);
}

ScopedFile::~ScopedFile()
{
    if (file_)
    {
        file_.close();
        SD.end();
    }
}

File &ScopedFile::get()
{
    return file_;
}

bool ScopedFile::isValid() const
{
    return file_;
}

void ScopedFile::open(const char *filename, FileMode mode, uint8_t sdPin, uint32_t frequency)
{
    if (!SD.begin(sdPin, SPI, frequency))
    {
        file_ = File();
        return;
    }
    file_ = SD.open(filename, (mode == FileMode::Write) ? FILE_WRITE : FILE_READ);
}
