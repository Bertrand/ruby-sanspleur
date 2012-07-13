/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#include "info_header.h"
#include "sampler.h"

InfoHeader::InfoHeader(const char *url_string, long usleep_int, const char *start_date, const char *extra_info_string)
{
	_url_string = sanspleur_copy_string(url_string);
	_usleep_int = usleep_int;
	_start_date = sanspleur_copy_string(start_date);
	_extra_info_string = sanspleur_copy_string(extra_info_string);
}

InfoHeader::~InfoHeader()
{
	if (_url_string) {
		free((void *)_url_string);
	}
	if (_start_date) {
		free((void *)_start_date);
	}
	if (_extra_info_string) {
		free((void *)_extra_info_string);
	}
}
		
InfoHeader *InfoHeader::copy() const
{
	return new InfoHeader(_url_string, _usleep_int, _start_date, _extra_info_string);
}

const char *InfoHeader::get_url_string() const
{
	return _url_string;
}

long InfoHeader::get_usleep_int() const
{
	return _usleep_int;
}

const char *InfoHeader::get_start_date() const
{
	return _start_date;
}

const char *InfoHeader::get_extra_info_string() const
{
	return _extra_info_string;
}
