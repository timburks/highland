(set @m_files     (filelist "^objc/.*.m$"))

(set SYSTEM ((NSString stringWithShellCommand:"uname") chomp))
(case SYSTEM
      ("Darwin"
	       (set @cflags "-g -O2 -std=gnu99 -DDARWIN ")
               (set @ldflags "-g -O2 -framework Foundation -framework Nu -lhighlander -lcrypto"))
      ("Linux"
              (set @arch (list "i386"))
              (set @cflags "-g -std=gnu99 -DLINUX -I/usr/include/GNUstep/Headers -I/usr/local/include -fconstant-string-class=NSConstantString ")
              (set @ldflags "-L/usr/local/lib -lNu -lhighlander -lcrypto"))
      (else nil))


(set @framework "Highland")
(set @framework_identifier "nu.programming.highland")

(compilation-tasks)
(framework-tasks)

(task "clobber" => "clean" is
      (SH "rm -rf #{@framework_dir}"))

(task "default" => "framework")

(task "doc" is (SH "nudoc"))

(task "install" => "framework" is
      (SH "sudo rm -rf /Library/Frameworks/#{@framework}.framework")
      (SH "sudo cp -rp #{@framework}.framework /Library/Frameworks/#{@framework}.framework"))

(task "test" => "framework" is
      (SH "nutest test/test_*.nu"))
