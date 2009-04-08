(load "template")

(global NSUTF8StringEncoding 4)


(set $template (NuTemplate codeForString:<<-TEMPLATE
<h1>Hello, getter!</h1>
<pre>
<%= (params description) %>
</pre>
TEMPLATE))



(get "/"
     (set params (REQUEST parameters))
     (eval $template))

;; front page.
(post "/"
(puts "got index")
     (set params (REQUEST parameters))
     (unless (params count) (set params (REQUEST postDictionary)))
     (set template <<-TEMPLATE
<h1>Hello, poster!</h1>
<pre>
<%= (params description) %>
</pre>
TEMPLATE)
     (eval (NuTemplate codeForString:template)))

(get "/foo"
     ((REQUEST parameters) description))

(post "/bar"
      (puts "bar handler")
      (puts "second")
      (puts ((context) description))
      (set data (REQUEST postData))
      (puts (data length))
      (set string ((NSString alloc) initWithData:data encoding:NSUTF8StringEncoding))
      (puts string)
      (puts ((REQUEST postDictionary) description))
      "<html><head></head><body><h2>highlander</h1></body></html>")

