require 'rubygems'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'rake/testtask'
require 'date'

# ------- Version ----
# Read version from header file
version_header = File.read('ext/ruby_sanspleur/version.h')
match = version_header.match(/RUBY_SANSPLEUR_VERSION\s*["](\d.+)["]/)
raise(RuntimeError, "Could not determine RUBY_SANSPLEUR_VERSION") if not match
RUBY_SANSPLEUR_VERSION = match[1]


# ------- Default Package ----------
FILES = FileList[
  'Rakefile',
  'README.rdoc',
  'LICENSE',
  'CHANGES',
  'bin/*',
  'ext/ruby_sanspleur/*.c',
  'ext/ruby_sanspleur/*.cpp',
  'ext/ruby_sanspleur/*.h',
  'lib/**/*',
  'rails/**/*',
]

# Default GEM Specification
default_spec = Gem::Specification.new do |spec|
  spec.name = "ruby-sanspleur"

  spec.homepage = "https://github.com/fotonauts/ruby-sanspleur"
  spec.summary = "Ruby Sampler"
  spec.description = "-"
  spec.version = RUBY_SANSPLEUR_VERSION

  spec.author = "-"
  spec.email = "-"
  spec.platform = Gem::Platform::RUBY
  spec.require_path = "lib"
  spec.bindir = "bin"
  spec.executables = ["ruby-sanspleur"]
  spec.extensions = ["ext/ruby_sanspleur/extconf.rb"]
  spec.files = FILES.to_a
  spec.required_ruby_version = '>= 1.8.4'
  spec.rubyforge_project = "-"
  spec.date = DateTime.now
  spec.add_development_dependency 'os'
  spec.add_development_dependency 'rake-compiler'

end


desc 'build native .gem files -- use like "native_gems clobber cross native gem"--for non native gem creation use "native_gems clobber" then "clean gem"'
task :native_gems do
  ENV['RUBY_CC_VERSION'] = '1.8.6:1.9.1'
  require 'rake/extensiontask'
  Rake::ExtensionTask.new('ruby_sanspleur', default_spec) do |ext|
    ext.cross_compile = true
    ext.cross_platform = ['x86-mswin32-60', 'x86-mingw32-60']
  end
end

# Rake task to build the default package
Rake::GemPackageTask.new(default_spec) do |pkg|
  pkg.need_tar = true
  #pkg.need_zip = true
end


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
