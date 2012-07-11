require 'test/unit'
require 'ruby-sanspleur'

class AClass
	def wait_a_little_bit
		sleep 0.01
	end

	def pouet
		self.wait_a_little_bit
	end

	def glu 
		(1..1000).each do |i|
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