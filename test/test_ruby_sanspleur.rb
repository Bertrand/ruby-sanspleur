require 'test/unit'
require 'ruby-sanspleur'

class AClass
	def self.wait_a_little_bit
		#sleep 0.0001
		a=1
		b = 1/2
	end

	def self.short_loop
		(1..10). each do 
			self.wait_a_little_bit
		end
	end

	def self.long_loop
		(1..1000000).each do |i|
			short_loop
		end
	end
end

class SansPleurTest < Test::Unit::TestCase
  def test_bonjour
    RubySanspleur.start_sample("pouet", 1000, "/tmp/glu.rubytrace", nil)
    AClass.long_loop
   	RubySanspleur.stop_sample("")
  end
end