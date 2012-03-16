require 'rubygems'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'rake/testtask'
require 'date'

# ---------  RDoc Documentation ------
desc "Generate rdoc documentation"
Rake::RDocTask.new("rdoc") do |rdoc|
end

task :default => :package

desc 'Run the ruby-sanspleur test suite'
Rake::TestTask.new do |t|
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
