require "./lib/ruby-sanspleur.rb"


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


t1=Time.now
RubySanspleur.start_sample("pouet", 5, "/tmp/glu.rubytrace", nil)
a = AClass.new
a.long_loop
RubySanspleur.stop_sample("")
t2=Time.now

puts "with sampling : #{t2-t1}"

t1=Time.now
a = AClass.new
a.long_loop
t2=Time.now
puts "without sampling : #{t2-t1}"