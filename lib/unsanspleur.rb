require "ruby-sanspleur"

at_exit {
  result = RubySanspleur.stop
  printer = RubySanspleur::FlatPrinter.new(result)
  printer.print(STDOUT)
}
RubySanspleur.start
