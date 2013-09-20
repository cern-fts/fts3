

function JobListCtrl($location, $scope, jobs, Job, Unique)
{
	// Jobs
	$scope.jobs = jobs;
	
	// Unique values
	$scope.unique = Unique.all();
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.jobs.page;
	$scope.pageCount = $scope.jobs.pageCount;

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
		vo:          undefinedAsEmpty($location.search().vo),
		source_se:   undefinedAsEmpty($location.search().source_se),
		dest_se:     undefinedAsEmpty($location.search().dest_se),
		time_window: undefinedAsEmpty($location.search().time_window),
		state:       statesFromString($location.search().state)
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:        1,
			vo:          validVo($scope.filter.vo),
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
    	
    	Job.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
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
    	
    	ArchivedJobs.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
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

	// We paginate the files
	$scope.pageMax    = 15;
	$scope.pageCount  = files.pageCount;
	$scope.page       = files.page;
	$scope.files      = files;
	
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	}
	
	// Filtering by state
	$scope.filter = {
		state:       statesFromString($location.search().state)
	}
	
	$scope.filterByState = function() {
		$location.search('state', joinStates($scope.filter.state));
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

    	Job.query({jobId: $route.current.params.jobId}, function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
    	},
    	function () {
    		deferred.resolve(false);
    		stopLoading($rootScope);
    		$location.path('/404');
    	});
    	
    	return deferred.promise;
    },

	files: function ($rootScope, $location, $route, $q, Files) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var filter = $location.search();
    	filter.jobId = $route.current.params.jobId
		Files.query(filter, function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		},
		function () {
			deferred.resolve(false);
			stopLoading($rootScope);
			$location.path('/404');
		});
    	
    	return deferred.promise;
    }
}
