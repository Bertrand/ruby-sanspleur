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
  spec.extensions << "ext/ruby_sanspleur/extconf.rb"

end