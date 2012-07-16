module RubySanspleur
  class Railtie < Rails::Railtie
    initializer "ruby_sanspleur.insert_middleware" do |app|
      app.config.middleware.use "::RubySanspleur::Middleware"
      Rails.logger.info "RubySanspleur Middleware loaded"
    end
  end
end