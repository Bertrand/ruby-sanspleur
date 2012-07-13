/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

class InfoHeader
{
	protected:
		char *_url_string;
		long _usleep_int;
		char *_start_date;
		char *_extra_info_string;
		
	public:
		InfoHeader(const char *url_string, long usleep_int, const char *start_date, const char *extra_info_string);
		~InfoHeader();
		
		InfoHeader *copy() const;
		
		const char *get_url_string() const;
		long get_usleep_int() const;
		const char *get_start_date() const;
		const char *get_extra_info_string() const;
};
