
function searchJob(jobList, jobId)
{
    for (j in jobList) {
        if (jobList[j].job_id == jobId)
            return jobList[j];
    }
    return {show: false};
}

function JobListCtrl($rootScope, $location, $scope, jobs, Job)
{
    // Jobs
    $scope.jobs = jobs;

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter = $location.$$search;
        filter.page = $scope.jobs.page;
        Job.query(filter, function(updatedJobs) {
            for (j in updatedJobs.items) {
                var job = updatedJobs.items[j];
                job.show = searchJob($scope.jobs.items, job.job_id).show;
            }
            $scope.jobs = updatedJobs;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });

    // Set up filters
    $scope.filter = {
        vo:          validString($location.$$search.vo),
        source_se:   validString($location.$$search.source_se),
        dest_se:     validString($location.$$search.dest_se),
        time_window: withDefault($location.$$search.time_window, 1),
        state:       statesFromString($location.$$search.state),
        diagnosis:   withDefault($location.$$search.diagnosis, 0),
        with_debug:  withDefault($location.$$search.with_debug, 0),
        multireplica:withDefault($location.$$search.multireplica, 0),
    }

    $scope.showFilterDialog = function() {
    	document.getElementById('filterDialog').style.display = 'block';
    }

    $scope.cancelFilters = function() {
    	document.getElementById('filterDialog').style.display = 'none';
    }

    $scope.applyFilters = function() {
    	document.getElementById('filterDialog').style.display = 'none';
        $location.search({
            page:        1,
            time_window: $scope.filter.time_window,
            state:       joinStates($scope.filter.state),
            diagnosis:   $scope.filter.diagnosis,
            with_debug:  $scope.filter.with_debug,
            multireplica:$scope.filter.multireplica
        });
    }

    // Method to set class depending on the metadata value
    $scope.classFromMetadata = function(job) {
        if (job.diagnosis) {
            return 'inconsistency';
        }
        var metadata = job.job_metadata;
        if (metadata) {
            metadata = eval('(' + metadata + ')');
            if (metadata && typeof(metadata) == 'object' && 'label' in metadata)
                return 'label-' + metadata.label;
        }
        return '';
    }
}


JobListCtrl.resolve = {
    jobs: function($rootScope, $location, $q, Job) {
        loading($rootScope);

        var deferred = $q.defer();

        var page = $location.$$search.page;
        if (!page || page < 1)
            page = 1;

        Job.query($location.$$search,
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}

/** Job view
 */
function JobViewCtrl($rootScope, $location, $scope, job, files, Job, Files)
{
    var page = $location.$$search.page;
    if (!page)
        page = 1;

    $scope.itemPerPage = 50;

    $scope.job = job;
    $scope.files = files;

    // Calculate remaining time
    $scope.getRemainingTime = function(file) {
    	if (file.file_state == 'ACTIVE') {
    		if (file.throughput && file.filesize) {
    			var bytes_per_sec = file.throughput * (1024 * 1024);
    			var remaining_bytes = file.filesize - file.transferred;
    			var remaining_time = remaining_bytes / bytes_per_sec;
    			return (Math.round(remaining_time*100)/100).toString() + ' s';
    		}
    		else {
    			return '?';
    		}
    	}
    	else {
    		return '-';
    	}
    }

    // On page change
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    }

    // Filtering
    $scope.filter = {
        state: statesFromString($location.$$search.state),
        reason: validString($location.$$search.reason),
        file: validString($location.$$search.file),
    }

    $scope.filterByState = function() {
        $location.search('state', joinStates($scope.filter.state));
    }

    $scope.resetReasonFilter = function() {
        $location.search({state: $location.$$search.state});
    }

    // Reloading
    $scope.autoRefresh = setInterval(function() {
        loading($rootScope);
        var filter   = $location.$$search;
        filter.jobId = $scope.job.job.job_id;
        Job.query(filter, function(updatedJob) {
            $scope.job = updatedJob;
        })
        // Do this in two steps so we can copy the show attribute
        Files.query(filter, function (updatedFiles) {
            for(var i = 0; i < updatedFiles.files.items.length; i++) {
                updatedFiles.files.items[i].show = $scope.files.files.items[i].show;
            }
            $scope.files = updatedFiles;
            stopLoading($rootScope);
        },
        genericFailureMethod(null, $rootScope, $location));
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
}


JobViewCtrl.resolve = {
    job: function ($rootScope, $location, $route, $q, Job) {
        loading($rootScope);

        var deferred = $q.defer();

        var filter = {
            jobId: $route.current.params.jobId
        };
        if ($route.current.params.file)
            filter.file = $route.current.params.file;
        if ($route.current.params.reason)
            filter.reason = $route.current.params.reason;

        Job.query(filter,
                  genericSuccessMethod(deferred, $rootScope),
                  genericFailureMethod(deferred, $rootScope));

        return deferred.promise;
    },

    files: function ($rootScope, $location, $route, $q, Files) {
        loading($rootScope);

        var deferred = $q.defer();

                var filter = $location.$$search;
        filter.jobId = $route.current.params.jobId
        filter.jobId = $route.current.params.jobId
        Files.query(filter,
              function (data) {
                genericSuccessMethod(deferred, $rootScope)(data);
                // If file filter is set, by default show the details
                if ($location.$$search.file) {
                    data.files.items[0].show = true;
                }
              },
              genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
