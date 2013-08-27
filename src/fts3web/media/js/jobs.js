

function JobListCtrl($location, $scope, jobs, Job, Unique)
{
	// Jobs
	$scope.jobs = jobs;
	
	// All vos
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
	}, 20000);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filter = {
		vo:        undefinedAsEmpty($location.search().vo),
		source_se: undefinedAsEmpty($location.search().source_se),
		dest_se:   undefinedAsEmpty($location.search().dest_se),
		time:      undefinedAsEmpty($location.search().time),
		state:     statesFromString($location.search().state)
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:      1,
			vo:        validVo($scope.filter.vo),
			source_se: $scope.filter.source_se,
			dest_se:   $scope.filter.dest_se,
			time:      $scope.filter.time,
			state:     joinStates($scope.filter.state)
		});
		$scope.filtersModal = false;
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

/** Job view
 * Note: We paginate files client-side
 **/
function JobViewSetFiles($location, $scope, page)
{
	var startIndex = (page - 1) * $scope.itemPerPage;
	var endIndex   = (page) * $scope.itemPerPage;
	
	if (endIndex > $scope.job.files.length)
		endIndex = $scope.job.files.length;
		
	$scope.files            = $scope.job.files.slice(startIndex, endIndex);
	$scope.files.startIndex = startIndex;
	$scope.files.endIndex   = endIndex; 
}

function JobViewCtrl($location, $scope, job, Job)
{
	var page = $location.search().page;
	if (!page)
		page = 1;
	
	$scope.itemPerPage = 50;
	
	$scope.job = job;
	
	$scope.pageMax    = 15;
	$scope.pageCount  = Math.ceil($scope.job.files.length / $scope.itemPerPage);
	$scope.page       = page ;
	
	JobViewSetFiles($location, $scope, page);
	
	$scope.pageChanged = function(newPage) {
		$location.search({page: newPage});
		JobViewSetFiles($location, $scope, newPage - 1);
	}
	
	// Reloading
	$scope.autoRefresh = setInterval(function() {
		var filter   = $location.search();
		filter.jobId = $scope.job.job_id;	
    	$scope.job   = Job.query(filter, function() {
    		var start = $scope.files.startIndex;
    		var end   = $scope.files.endIndex;
    		$scope.files = $scope.job.files.slice(start, end);
    		$scope.files.startIndex = start;
    		$scope.files.endIndex   = end;
    	});
	}, 20000);
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
    	});
    	
    	return deferred.promise;
    }
}


/**
 * Job queue
 */
function JobQueueCtrl($location, $scope, jobs, Job, Unique) {
	
}


JobQueueCtrl.resolve = {
		
}
