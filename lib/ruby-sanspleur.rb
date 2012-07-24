# require the  .so file...

me = File.dirname(__FILE__) + '/'
begin
  # fat binaries
  require "#{me}/#{RUBY_VERSION[0..2]}/ruby_sanspleur"
rescue Exception
  require "#{me}/../ext/ruby_sanspleur/ruby_sanspleur"
end


require "ruby-sanspleur/errors"
require "ruby-sanspleur/middleware"

begin
	require "ruby-sanspleur/railtie" if defined?(Rails) 
rescue Exception => ex
	Logger.new(STDERR).error("Unable to load ruby_sanspleur railtie (#{ex.inspect}).")
end
