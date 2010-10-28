# require the  .so file...

me = File.dirname(__FILE__) + '/'
begin
  # fat binaries
  require "#{me}/#{RUBY_VERSION[0..2]}/ruby_sanspleur"
rescue Exception
  require "#{me}/../ext/ruby_sanspleur/ruby_sanspleur"
end
