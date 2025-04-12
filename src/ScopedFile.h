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
#ifndef SCOPEDFILE_H
#define SCOPEDFILE_H

#include <Arduino.h>
#include <SD.h>

enum class FileMode
{
    Read,
    Write
};

class ScopedFile
{
public:


    ScopedFile(const char *filename, FileMode mode = FileMode::Read, uint8_t sdPin = SS, uint32_t frequency = 4000000);
    ScopedFile(const String &filename, FileMode mode = FileMode::Read, uint8_t sdPin = SS, uint32_t frequency = 4000000);
    ~ScopedFile();

    File &get();
    bool isValid() const;

    ScopedFile(const ScopedFile &) = delete;
    ScopedFile &operator=(const ScopedFile &) = delete;
    ScopedFile(ScopedFile &&) = delete;
    ScopedFile &operator=(ScopedFile &&) = delete;

private:
    File file_;
    void open(const char *filename, FileMode mode, uint8_t sdPin, uint32_t frequency);
};

#endif // SCOPEDFILE_H