require 'timeout'

module RubySanspleur  
  class Middleware
    TRUE_VALUES = ["1", "true", "yes"]
    VALID_RUBY_SANSPLEUR_OPTIONS = [:ruby_sanspleur_interval, :ruby_sanspleur_timeout]

    def initialize(app, options = {})
      @app = app
    end
    
    def logger
        logger = Rails.logger
    end

    def call(env)
      sampling_enabled = false

      # quickly discard normal requests
      if env["QUERY_STRING"].index('ruby_sanspleur') then 

        logger.info { "sampling request #{env["PATH_INFO"]}" } if logger
        query_hash = Rack::Utils::parse_query(env["QUERY_STRING"]).symbolize_keys! rescue nil

        if (query_hash && TRUE_VALUES.include?(query_hash[:ruby_sanspleur])) then 

          # check auth
          raise InvalidSecret unless self.request_authorized?(env)

          # compute params
          defaults_options = {
            :ruby_sanspleur_interval => 10
          }
          options = defaults_options.merge(query_hash.slice(*VALID_RUBY_SANSPLEUR_OPTIONS))
          microseconds_interval = options[:ruby_sanspleur_interval].to_i * 1000 
          timeout = options[:ruby_sanspleur_timeout] && options[:ruby_sanspleur_timeout].to_f

          # safe dirty way to get a temporary filename.  
          tmpfile = Tempfile.new("profile.rubytrace")
          tracefile_path = tmpfile.path
          tmpfile.close(true) # force deletion

          # start sampling
          RubySanspleur.start_sample(env['HTTP_HOST'] + env["REQUEST_URI"], microseconds_interval, tracefile_path, nil)
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
          logger.warn { "got an exception while sampling: #{e}" } if logger
        ensure
          # there are situations where we cannot catch the exception, but ensure is always called...
          RubySanspleur.stop_sample(nil)
        end

        logger.debug { "finalize sampling" } if logger
        response = ::File.open(tracefile_path, "r")
        headers = {"Cache-Control" => "no-cache", "Content-Type" => "application/rubytrace", "Content-Disposition" => 'attachment; filename="profile.rubytrace"'}
        status = 200
        logger.debug { "sampling done" } if logger
        
        [ status, headers, response ]
      else
        @app.call(env)
      end
    end # call

    def request_authorized?(env)
      self.delegate_with_env_or_execute(:should_allow_sampling_request_for_environment, env) do
        logger.info { "original url : " + original_request_uri(env) }
        computed_signature =  OpenSSL::HMAC.hexdigest('sha1', self.secret_key, self.original_request_uri(env))
        transmitted_signature = env["HTTP_X_RUBY_SANSPLEUR_SIGNATURE"]
        transmitted_signature && computed_signature && (computed_signature.casecmp(transmitted_signature) == 0)        
      end
    end

    def secret_key
      secret_hex_key = self.value_for_config_key(:secret_hex_key)
      raise InvalidConfiguration.new("Invalid configuration: no secret key provided.") if secret_hex_key.nil?
      Array(secret_hex_key).pack('H*') 
    end

    def original_request_uri(env)
      self.delegate_with_env_or_execute(:orignal_uri_for_environment, env) do
        url_scheme = env["rack.url_scheme"]
        host_name = env["HTTP_HOST"] || env["SERVER_NAME"] # fallback to SERVER_NAME will not work if there is an explicit HTTP port in the request
        query_string = env["QUERY_STRING"] && env["QUERY_STRING"].split('&').sort.join('&') # sort query string parameters without touching to encoding.
        full_path = env["PATH_INFO"] + (query_string.empty? ? "" : ("?" + query_string))
        url_scheme + "://" + host_name + full_path
      end
    end

    # config utils

    def value_for_config_key(key)
      raw_value = self._raw_value_for_config_key(key)
      raw_value.is_a?(Proc) ? raw_value.call : raw_value
    end

    def delegate_proc_for_config_key(key)
      config_value = self._raw_value_for_config_key(key)
      config_value.is_a?(Proc) && config_value
    end

    def delegate_with_env_or_execute(delegate_method_name, env, &block)
      delegate_proc = delegate_proc_for_config_key(delegate_method_name)
      if delegate_proc then 
        delegate_proc.call(env)
      else
        block.call
      end
    end

    def _raw_value_for_config_key(key)
      value = Rails.application.config.ruby_sanspleur[key]
      logger.debug "config for key #{key} is #{value}"
      value
    end

  end # Middleware
  
end # RubySanspleur
