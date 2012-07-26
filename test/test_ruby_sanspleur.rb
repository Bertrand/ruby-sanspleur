require 'test/unit'
require 'ruby-sanspleur'


module AModule
	def long_loop
		(1..1000000).each do |i|
			AClass.short_loop
		end
	end
end

class AClass

	include AModule

	def self.wait_a_little_bit
		a=1
		b = 1/2
	end

	def self.short_loop
		(1..10). each do 
			self.wait_a_little_bit
		end
	end

end


class SansPleurTest < Test::Unit::TestCase
  def test_bonjour
    RubySanspleur.start_sample("pouet", 100000, "/tmp/glu.rubytrace", nil)
    a = AClass.new
    a.long_loop
   	RubySanspleur.stop_sample("")
  end
end