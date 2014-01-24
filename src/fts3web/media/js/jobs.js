

function JobListCtrl($location, $scope, jobs, Job, Unique)
{
    // Jobs
    $scope.jobs = jobs;
    
    // Unique values
    $scope.unique = {
        sources: Unique('sources'),
        destinations: Unique('destinations'),
        vos: Unique('vos')
    }

    // On page change, reload
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    };

    // Set timer to trigger autorefresh
    $scope.autoRefresh = setInterval(function() {
        var filter = $location.search();
        filter.page = $scope.jobs.page;
        $scope.jobs = Job.query(filter);
    }, REFRESH_INTERVAL);
    $scope.$on('$destroy', function() {
        clearInterval($scope.autoRefresh);
    });
    
    // Set up filters
    $scope.filter = {
        vo:          validString($location.search().vo),
        source_se:   validString($location.search().source_se),
        dest_se:     validString($location.search().dest_se),
        time_window: validString($location.search().time_window),
        state:       statesFromString($location.search().state)
    }
    
    $scope.applyFilters = function() {
        $location.search({
            page:        1,
            vo:          validString($scope.filter.vo),
            source_se:   $scope.filter.source_se,
            dest_se:     $scope.filter.dest_se,
            time_window: $scope.filter.time_window,
            state:       joinStates($scope.filter.state)
        });
        $scope.filtersModal = false;
    }
    
    // Method to set class depending on the metadata value
    $scope.classFromMetadata = function(job) {
        var metadata = job.job_metadata;
        if (metadata) {
            metadata = eval('(' + metadata + ')');
            if ('label' in metadata)
                return 'label-' + metadata.label;
        }
        return '';
    }
}


JobListCtrl.resolve = {
    jobs: function($rootScope, $location, $q, Job) {
        loading($rootScope);
        
        var deferred = $q.defer();

        var page = $location.search().page;
        if (!page || page < 1)
            page = 1;
        
        Job.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));
        
        return deferred.promise;
    }
}

/** Archive */
function ArchiveCtrl($location, $scope, jobs, ArchivedJobs, Unique)
{
    JobListCtrl($location, $scope, jobs, ArchivedJobs, Unique);
}

ArchiveCtrl.resolve = {
    jobs: function($rootScope, $location, $q, ArchivedJobs) {
        loading($rootScope);
        
        var deferred = $q.defer();

        var page = $location.search().page;
        if (!page || page < 1)
            page = 1;
        
        ArchivedJobs.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
              genericFailureMethod(deferred, $rootScope, $location));
        
        return deferred.promise;
    }
}

/** Job view
 */
function JobViewCtrl($location, $scope, job, files, Job, Files)
{
    var page = $location.search().page;
    if (!page)
        page = 1;
    
    $scope.itemPerPage = 50;
    
    $scope.job = job;
    $scope.files = files;
    
    // On page change
    $scope.pageChanged = function(newPage) {
        $location.search('page', newPage);
    }
    
    // Filtering
    $scope.filter = {
        state: statesFromString($location.search().state),
        reason: validString($location.search().reason),
        file: validString($location.search().file),
    }
    
    $scope.filterByState = function() {
        $location.search('state', joinStates($scope.filter.state));
    }
    
    $scope.resetReasonFilter = function() {
        $location.search({state: $location.search().state});
    }
    
    // Reloading
    $scope.autoRefresh = setInterval(function() {
        var filter   = $location.search();
        filter.jobId = $scope.job.job.job_id;   
        Job.query(filter, function(updatedJob) {
            $scope.job = updatedJob;
        })
        // Do this in two steps so we can copy the show attribute
        Files.query(filter, function (updatedFiles) {
            for(var i = 0; i < updatedFiles.items.length; i++) {
                updatedFiles.items[i].show = $scope.files.items[i].show;
            }
            $scope.files = updatedFiles;
        });
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
            jobId: $route.current.params.jobId,
            archive: validString($route.current.params.archive)
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

        var filter = $location.search();
        filter.jobId = $route.current.params.jobId
        Files.query(filter,
              function (data) {
                genericSuccessMethod(deferred, $rootScope)(data);
                // If file filter is set, by default show the details
                if ($location.search().file) {
                    data.items[0].show = true;
                }
              },
              genericFailureMethod(deferred, $rootScope, $location));
        
        return deferred.promise;
    }
}
