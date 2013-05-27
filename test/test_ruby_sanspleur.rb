require 'test/unit'
require 'ruby-sanspleur'


module AModule
	def long_loop
		(1..100000).each do |i|
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
		(1..20). each do 
			self.wait_a_little_bit
		end
	end

end


class SansPleurTest < Test::Unit::TestCase
  def test_bonjour
    RubySanspleur.start_sample("pouet", 1000, "/tmp/glu.rubytrace", nil)
    a = AClass.new
    a.long_loop
   	RubySanspleur.stop_sample("")
  end
end