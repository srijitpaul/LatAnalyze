/*
 * AsciiFile.cpp, part of LatAnalyze 3
 *
 * Copyright (C) 2013 - 2014 Antonin Portelli
 *
 * LatAnalyze 3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LatAnalyze 3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LatAnalyze 3.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <latan/AsciiFile.hpp>
#include <latan/includes.hpp>

using namespace std;
using namespace Latan;

/******************************************************************************
 *                        AsciiFile implementation                            *
 ******************************************************************************/
// AsciiParserState constructor ////////////////////////////////////////////////
AsciiFile::AsciiParserState::AsciiParserState(istream *stream, string *name,
                                              IoDataTable *data)
: ParserState<IoDataTable>(stream, name, data)
{
    initScanner();
}

// AsciiParserState destructor /////////////////////////////////////////////////
AsciiFile::AsciiParserState::~AsciiParserState(void)
{
    destroyScanner();
}

// constructor /////////////////////////////////////////////////////////////////
AsciiFile::AsciiFile(void)
: File(), fileStream_()
, isParsed_(false)
, state_(nullptr)
{}

AsciiFile::AsciiFile(const string &name, const unsigned int mode)
{
    open(name, mode);
}

// destructor //////////////////////////////////////////////////////////////////
AsciiFile::~AsciiFile(void)
{
    close();
}

// access //////////////////////////////////////////////////////////////////////
void AsciiFile::save(const DMat &m, const std::string &name)
{
    checkWritability();
    isParsed_ = false;
    fileStream_ << "#L latan_begin mat " << name << endl;
    fileStream_ << m.cols() << endl;
    fileStream_ << scientific << m << endl;
    fileStream_ << "#L latan_end mat " << endl;
}

void AsciiFile::save(const DMatSample &s, const std::string &name)
{
    checkWritability();
    isParsed_ = false;
    fileStream_ << "#L latan_begin rs_sample " << name << endl;
    fileStream_ << s.size() << endl;
    save(s[central], name + "_C");
    for (Index i = 0; i < s.size(); ++i)
    {
        save(s[i], name + "_S_" + strFrom(i));
    }
    fileStream_ << "#L latan_end rs_sample " << endl;
}

void AsciiFile::save(const RandGenState &state, const std::string &name)
{
    checkWritability();
    isParsed_ = false;
    fileStream_ << "#L latan_begin rg_state " << name << endl;
    fileStream_ << state << endl;
    fileStream_ << "#L latan_end rg_state " << endl;
}

// tests ///////////////////////////////////////////////////////////////////////
bool AsciiFile::isOpen() const
{
    return fileStream_.is_open();
}

// IO //////////////////////////////////////////////////////////////////////////
void AsciiFile::close(void)
{
    state_.reset();
    if (isOpen())
    {
        fileStream_.close();
    }
    name_     = "";
    mode_     = Mode::null;
    isParsed_ = false;
    deleteData();
}

void AsciiFile::open(const string &name, const unsigned int mode)
{
    if (isOpen())
    {
        LATAN_ERROR(Io, "file already opened with name '" + name_ + "'");
    }
    else
    {
        ios_base::openmode stdMode = static_cast<ios_base::openmode>(0);
        
        if (mode & Mode::write)
        {
            stdMode |= ios::out|ios::trunc;
        }
        if (mode & Mode::read)
        {
            stdMode |= ios::in;
        }
        if (mode & Mode::append)
        {
            stdMode |= ios::out|ios::app;
        }
        name_     = name;
        mode_     = mode;
        isParsed_ = false;
        fileStream_.open(name_.c_str(), stdMode);
        if (mode_ & Mode::read)
        {
            state_.reset(new AsciiParserState(&fileStream_, &name_, &data_));
        }
        else
        {
            state_.reset();
        }
    }
}

std::string AsciiFile::load(const string &name)
{
    if ((mode_ & Mode::read)&&(isOpen()))
    {
        if (!isParsed_)
        {
            state_->isFirst = true;
            parse();
        }
        if (name.empty())
        {
            return state_->first;
        }
        else
        {
            return name;
        }
    }
    else
    {
        if (isOpen())
        {
            LATAN_ERROR(Io, "file '" + name_ + "' is not opened in read mode");
        }
        else
        {
            LATAN_ERROR(Io, "file not opened");
        }
    }
}

// parser //////////////////////////////////////////////////////////////////////

//// Bison/Flex parser declaration
int _Ascii_parse(AsciiFile::AsciiParserState *state);

void AsciiFile::parse()
{
    fileStream_.seekg(0);
    _Ascii_parse(state_.get());
    isParsed_ = true;
}