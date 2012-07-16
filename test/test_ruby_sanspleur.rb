require 'test/unit'
require 'ruby-sanspleur'

class AClass
	def wait_a_little_bit
		#sleep 0.0001
		a=1
		b = 1/2
	end

	def pouet
		(1..10). each do 
			self.wait_a_little_bit
		end
	end

	def glu 
		(1..1000000).each do |i|
			self.pouet
		end
	end
end

class SansPleurTest < Test::Unit::TestCase
  def test_bonjour
    RubySanspleur.start_sample("pouet", 100000, "/tmp/glu", nil)
    c = AClass.new
    c.glu
   	RubySanspleur.stop_sample("")
  end
end