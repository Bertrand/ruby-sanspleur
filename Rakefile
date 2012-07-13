require 'rubygems'
require 'rubygems/package_task'
#require 'rake/rdoctask'
require 'rake/testtask'
require 'date'

# ---------  RDoc Documentation ------
# desc "Generate rdoc documentation"
# Rake::RDocTask.new("rdoc") do |rdoc|
# end

task :default => :package

NAME="ruby_sanspleur"

desc 'Run the ruby-sanspleur test suite'
Rake::TestTask.new do |t|
    t.libs << 'test'
end

require 'fileutils'

desc 'Build ruby_sanspleur.so'
task :build do
 Dir.chdir('ext/ruby_sanspleur') do
  unless File.exist? 'Makefile'
    system(Gem.ruby + " extconf.rb")
    system("make clean")
  end
  system("make")
  FileUtils.cp 'ruby_sanspleur.so', '../../lib' if File.exist? 'lib/ruby_sanspleur.so'
  FileUtils.cp 'ruby_sanspleur.bundle', '../../lib' if File.exist? 'lib/ruby_sanspleur.bundle'
 end
end

file "lib/#{NAME}/#{NAME}.so" =>
  Dir.glob("ext/#{NAME}/*{.rb,.c}") do
    Dir.chdir("ext/#{NAME}") do
      # this does essentially the same thing
      # as what RubyGems does
      ruby "extconf.rb"
      sh "make"
    end
    cp "ext/#{NAME}/#{NAME}.so", "lib/#{NAME}"
  end

task :test => :build

desc 'clean stuff'
task :cleanr do
 FileUtils.rm 'lib/ruby_sanspleur.so' if File.exist? 'lib/ruby_sanspleur.so'
 FileUtils.rm 'lib/ruby_sanspleur.bundle' if File.exist? 'lib/ruby_sanspleur.bundle'
 Dir.chdir('ext/ruby_sanspleur') do
  if File.exist? 'Makefile'
    system("make clean")
    FileUtils.rm 'Makefile'
  end
  Dir.glob('*~') do |file|
    FileUtils.rm file
  end
 end
 system("rm -rf pkg")
end

task :clean => :cleanr
task :package => :build