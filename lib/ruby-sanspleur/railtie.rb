module RubySanspleur
  class Railtie < Rails::Railtie

  	config.ruby_sanspleur = ActiveSupport::OrderedOptions.new
	config.action_dispatch.rescue_responses['RubySanspleur::InvalidSecret'] = :unauthorized

    initializer "ruby_sanspleur.insert_middleware" do |app|
      app.config.middleware.use "::RubySanspleur::Middleware"
      Rails.logger.info "RubySanspleur Middleware loaded"
    end
  end
end