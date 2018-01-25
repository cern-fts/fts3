# -*- mode: ruby -*-
# vi: set ft=ruby :
#
# Look at README.md to see how to use this file
#

require 'getoptlong'

# By default uses centos/7. You can do `vagrant --box=centos/6`
opts = GetoptLong.new(
    ['--box', GetoptLong::OPTIONAL_ARGUMENT]
)

box = "bento/centos-7.4"

opts.each do |opt, arg|
    case opt
        when '--box'
            box = arg
    end
end

Vagrant.configure("2") do |config|
    config.vm.box = box
    #config.vm.provision "file", source: "packaging/rpm/fts.spec", destination: "/tmp/fts.spec"
    config.vm.provision :shell, path: "bootstrap.sh"

    config.vm.provider "virtualbox" do |vb|
        vb.memory = 1024
        vb.cpus = 2
    end
end

