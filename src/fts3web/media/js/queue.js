function JobQueueCtrl($location, $scope, queue, QueuePairs, Unique)
{
	// Queue entries
	$scope.queue = queue;
	
	// Unique pairs and vos
	$scope.unique = Unique.all();
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.queue.page;
	$scope.pageCount = $scope.queue.pageCount;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.queue.page;
    	$scope.queue = QueuePairs.query(filter);
	}, 20000);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filter = {
		vo:        undefinedAsEmpty($location.search().vo),
		source_se: undefinedAsEmpty($location.search().source_se),
		dest_se:   undefinedAsEmpty($location.search().dest_se)
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:      1,
			vo:        validVo($scope.filter.vo),
			source_se: $scope.filter.source_se,
			dest_se:   $scope.filter.dest_se
		});
		$scope.filtersModal = false;
	}	
}


JobQueueCtrl.resolve = {
	queue: function($rootScope, $location, $q, QueuePairs) {
		loading($rootScope);
		
		var deferred = $q.defer();
	
		var page = $location.search().page;
		if (!page || page < 1)
			page = 1;
		
		QueuePairs.query($location.search(), function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	}
}



function JobQueueDetailedCtrl($location, $scope, queue, Queue)
{
	// Queue entries
	$scope.queue = queue;
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.queue.page;
	$scope.pageCount = $scope.queue.pageCount;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.queue.page;
    	$scope.queue = QueuePairs.query(filter);
	}, 20000);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filter = {
		vo:        undefinedAsEmpty($location.search().vo),
		source_se: undefinedAsEmpty($location.search().source_se),
		dest_se:   undefinedAsEmpty($location.search().dest_se)
	}
	
	$scope.applyFilters = function() {
		$location.search({
			page:      1,
			vo:        validVo($scope.filter.vo),
			source_se: $scope.filter.source_se,
			dest_se:   $scope.filter.dest_se
		});
		$scope.filtersModal = false;
	}	
}


JobQueueDetailedCtrl.resolve = {
	queue: function($rootScope, $location, $q, Queue) {
		loading($rootScope);
		
		var deferred = $q.defer();
	
		var page = $location.search().page;
		if (!page || page < 1)
			page = 1;
		
		Queue.query($location.search(), function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	}
}
