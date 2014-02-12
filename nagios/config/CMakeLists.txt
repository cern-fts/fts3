cmake_minimum_required(VERSION 2.8)

file (GLOB fts_probes_config fts3-template.cfg)

install (FILES ${fts_probes_config}
         DESTINATION etc/nagios/objects/
)

