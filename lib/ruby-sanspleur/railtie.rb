module RubySanspleur
  class Railtie < Rails::Railtie

  	config.ruby_sanspleur = ActiveSupport::OrderedOptions.new
	config.action_dispatch.rescue_responses['RubySanspleur::InvalidSecret'] = :unauthorized

  end
end