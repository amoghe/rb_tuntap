require "bundler/gem_tasks"
require 'rake/extensiontask'

Rake::ExtensionTask.new('rb_tuntap_ext')

desc "Open an irb session preloaded with this library"
task :console do
  top_dir = File.dirname(__FILE__)
  lib_dir = File.join(top_dir, 'lib')

  exec "irb -I #{lib_dir} -r rb_tuntap -r irb/completion"
end
