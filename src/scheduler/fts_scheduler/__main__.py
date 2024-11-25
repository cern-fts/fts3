#!/usr/bin/python3

import argparse
import configparser
import importlib
import logging
import logging.handlers
import os
import socket
import sys
import time

from db import DbConnPool
from scheduler import Scheduler


def get_log(log_file_path, log_program_name, log_level):
    hostname = socket.gethostname()

    log_format = (
        "%(asctime)s.%(msecs)03d000 "
        + hostname
        + " %(levelname)s "
        + log_program_name
        + ':LVL="%(levelname)s" PID="%(process)d" TID="%(process)d" MSG="%(message)s"'
    )
    log_date_format = "%Y/%m/%d %H:%M:%S"
    log_formatter = logging.Formatter(fmt=log_format, datefmt=log_date_format)

    log_dir = os.path.dirname(log_file_path)
    if not os.path.isdir(log_dir):
        raise Exception(
            "The logging directory {} is not a directory or does not exist".format(
                log_dir
            )
        )
    if not os.access(log_dir, os.W_OK):
        raise Exception("The logging directory {} cannot be written to".format(log_dir))

    log_handler = logging.handlers.TimedRotatingFileHandler(
        filename=log_file_path, when="midnight", backupCount=30
    )
    log_handler.setFormatter(log_formatter)

    log = logging.getLogger()
    log.setLevel(log_level)
    log.addHandler(log_handler)

    return log


def get_config(path, program_name):
    if not os.path.isfile(path):
        print(f"The {program_name} configuration file does not exist: path={path}")
        exit(1)

    config = configparser.ConfigParser()
    config.read(path)

    if not config.has_option("database", "user"):
        raise Exception(
            f"Missing configuration option: option=database.user path={path}"
        )

    if not config.has_option("database", "password"):
        raise Exception(
            f"Missing configuration option: option=database.password path={path}"
        )

    if not config.has_option("database", "dbname"):
        raise Exception(
            f"Missing configuration option: option=database.dbname path={path}"
        )

    if not config.has_option("database", "host"):
        raise Exception(
            f"Missing configuration option: option=database.host path={path}"
        )

    if not config.has_option("database", "port"):
        raise Exception(
            f"Missing configuration option: option=database.port path={path}"
        )

    if config.has_option("log", "level"):
        valid_log_levels = ["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
        log_level = config.get("log", "level")
        if log_level not in valid_log_levels:
            raise Exception(
                f"Invalid database log-level: permitted={valid_log_levels} actual={log_level}"
            )

    result = {
        "log_file": config.get(
            "log", "file", fallback=f"/var/log/fts4/{program_name}.log"
        ),
        "log_level": config.get("log", "level", fallback="INFO"),
        "sec_between_executions": config.getint(
            "main", "sec_between_executions", fallback=2
        ),
        "db_user": config.get("database", "user"),
        "db_password": config.get("database", "password"),
        "db_dbname": config.get("database", "dbname"),
        "db_host": config.get("database", "host"),
        "db_port": config.getint("database", "port"),
        "db_sslmode": config.get("database", "sslmode", fallback="require"),
        "scheduler_algo_module_name": config.get(
            "scheduler", "algo_module_name", fallback="default_scheduler_algo"
        ),
        "scheduler_algo_class_name": config.get(
            "scheduler", "algo_class_name", fallback="DefaultSchedulerAlgo"
        ),
    }

    return result


def get_major_db_schema_version(dbconn):
    with dbconn.cursor() as cursor:
        cursor.execute("SELECT MAX(major) FROM  t_schema_vers")
        rows = cursor.fetchall()
        major_schema_version = rows[0][0]
        return major_schema_version


program_name = "fts_scheduler"

parser = argparse.ArgumentParser(description="Schedules FTS transfer jobs")
parser.add_argument(
    "-c",
    "--config",
    default=f"/etc/fts4/{program_name}.ini",
    help="Path of the configuration file",
)

cmd_line = parser.parse_args()

config = get_config(cmd_line.config, program_name)

log = get_log(
    log_file_path=config["log_file"],
    log_program_name=program_name,
    log_level=config["log_level"],
)

log.info("Started")

nb_dbconns = 1
dbconn_pool = DbConnPool(
    min_conn=nb_dbconns,
    max_conn=nb_dbconns,
    host=config["db_host"],
    port=config["db_port"],
    db_name=config["db_dbname"],
    user=config["db_user"],
    password=config["db_password"],
    sslmode=config["db_sslmode"],
)

min_allowed_major_db_schema_version = 9
with dbconn_pool.get_dbconn() as dbconn:
    actual_major_db_schema_version = get_major_db_schema_version(dbconn)
if actual_major_db_schema_version < min_allowed_major_db_schema_version:
    print(
        f"Aborting: "
        "Database major version number is less than required: "
        f"min_allowed={min_allowed_major_db_schema_version} "
        f"actual={actual_major_db_schema_version}"
    )
    exit(1)

try:
    sys.path.append("/usr/share/fts/fts_scheduler_algo")
    try:
        scheduler_algo_module = importlib.import_module(
            config["scheduler_algo_module_name"]
        )
    except Exception as e:
        raise Exception(f"Failed to import module: {e}")

    SchedulerAlgoClass = getattr(
        scheduler_algo_module, config["scheduler_algo_class_name"], None
    )
    if SchedulerAlgoClass is None:
        raise Exception("Failed to find class")
except Exception as e:
    raise Exception(
        "Failed to import the scheduler-algorithm class: "
        f"module={config['scheduler_algo_module_name']} "
        f"class={config['scheduler_algo_class_name']}: "
        f"{e}"
    )

scheduler = Scheduler(log, SchedulerAlgoClass)

while True:
    execution_start = time.time()

    try:
        with dbconn_pool.get_dbconn() as dbconn:
            scheduler.schedule(dbconn)
    except Exception as e:
        log.error(f"Caught exception when trying to schedule: {e}")
        import traceback

        traceback.print_exc()

    execution_duration = time.time() - execution_start

    secs_to_next_execution = config["sec_between_executions"] - execution_duration
    if secs_to_next_execution > 0:
        time.sleep(secs_to_next_execution)
