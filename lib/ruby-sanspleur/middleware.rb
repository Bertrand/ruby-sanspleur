module RubySanspleur
  
  class Middleware
    TRUE_VALUES = ["1", "true", "yes"]
    VALID_RUBY_SANSPLEUR_OPTIONS = [:interval]

    def initialize(app, options = {})
      @app = app
    end
    
    def call(env)
      sampling_enabled = false

      if env["QUERY_STRING"].index('ruby_sanspleur') then # quickly discard normal requests
        query_hash = Rack::Utils::parse_query(env["QUERY_STRING"]).symbolize_keys! rescue nil
        if (query_hash && TRUE_VALUES.include?(query_hash[:ruby_sanspleur])) then 
          defaults_options = {
            :interval => 100000
          }

          options = defaults_options.merge(query_hash.slice(*VALID_RUBY_SANSPLEUR_OPTIONS))
          interval = options[:interval].to_i

          # safe dirty way to get a temporary filename.  
          tmpfile = Tempfile.new("profile.rubytrace")
          tracefile_path = tmpfile.path
          tmpfile.close(true) # force deletion

          RubySanspleur.start_sample(env['HTTP_HOST'] + env["REQUEST_URI"], interval, tracefile_path, nil)
          sampling_enabled = true
        end
      end

      begin
        status, headers, response = @app.call(env)
      ensure
        RubySanspleur.stop_sample(nil) if sampling_enabled
      end

      if sampling_enabled then
        response = ::File.open(tracefile_path, "r")
        headers = {"Cache-Control" => "no-cache", "Content-Type" => "application/rubytrace", "Content-Disposition" => 'attachment; filename="profile.rubytrace"'}
        status = 200
      end
      
      [ status, headers, response ]
    end # call

  end # Middleware
  
end # RubySanspleur
