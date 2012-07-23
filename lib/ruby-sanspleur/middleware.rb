require 'timeout'
module RubySanspleur
  
  class Middleware
    TRUE_VALUES = ["1", "true", "yes"]
    VALID_RUBY_SANSPLEUR_OPTIONS = [:interval, :timeout]

    def initialize(app, options = {})
      @app = app
    end
    
    def call(env)
      sampling_enabled = false

      if env["QUERY_STRING"].index('ruby_sanspleur') then # quickly discard normal requests
        logger = env["rack.logger"]
        logger.info { "sampling request #{env["PATH_INFO"]}" } if logger
        query_hash = Rack::Utils::parse_query(env["QUERY_STRING"]).symbolize_keys! rescue nil
        if (query_hash && TRUE_VALUES.include?(query_hash[:ruby_sanspleur])) then 
          defaults_options = {
            :interval => 100000
          }

          hexkey = "bebba81c39faa76ddadf36432bc0dfb67bb4fb7816fcdd6dffc294d4d4d620b16f0a7a5dae8fd99405a7ed61db4c1d0e320ca47c908bedb4cf3730ae4fcf2c25"
          key = Array(hexkey).pack('H*')

          url_scheme = env["rack.url_scheme"]
          host_name = env["HTTP_HOST"] || env["SERVER_NAME"] # fallback to SERVER_NAME will not work if using an explicit HTTP port in the request
          full_path = env["ORIGINAL_FULLPATH"] 
          if (full_path.nil?)
            full_path = env["PATH_INFO"] + (env["QUERY_STRING"].empty? ? "" : ("?" + env["QUERY_STRING"]))
          end
          original_url = url_scheme + "://" + host_name + full_path

          signature =  OpenSSL::HMAC.hexdigest('sha1', key, original_url)

          # XXX  - check signature
          
          options = defaults_options.merge(query_hash.slice(*VALID_RUBY_SANSPLEUR_OPTIONS))
          interval = options[:interval].to_i
          timeout = options[:timeout] && options[:timeout].to_f

          # safe dirty way to get a temporary filename.  
          tmpfile = Tempfile.new("profile.rubytrace")
          tracefile_path = tmpfile.path
          tmpfile.close(true) # force deletion

          RubySanspleur.start_sample(env['HTTP_HOST'] + env["REQUEST_URI"], interval, tracefile_path, nil)
          sampling_enabled = true
        end
      end

      if sampling_enabled
        begin
          if timeout
            logger.debug { "sampling with timeout #{timeout}" } if logger
            Timeout.timeout(timeout)  { @app.call(env) }
          else
            logger.debug { "sampling without timeout" } if logger
            @app.call(env) 
          end
        rescue => e
          logger.warn { "got an exception while sampling #{e}" } if logger
        ensure
          # there are situations where we cannot catch the exception but ensure are called...
          RubySanspleur.stop_sample(nil)
        end

        # logger.debug { "finalize sampling" } if logger
        response = ::File.open(tracefile_path, "r")
        headers = {"Cache-Control" => "no-cache", "Content-Type" => "application/rubytrace", "Content-Disposition" => 'attachment; filename="profile.rubytrace"'}
        status = 200
        # logger.debug { "sampling done" } if logger
        
        [ status, headers, response ]
      else
        @app.call(env)
      end
    end # call

  end # Middleware
  
end # RubySanspleur
