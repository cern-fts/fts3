
function OptimizerCtrl($location, $scope, optimizer, Optimizer, Unique)
{
	$scope.optimizer = optimizer;

	// Unique values
	$scope.unique = {
		sources: Unique('sources'),
		destinations: Unique('destinations')
	}

	// Filter
	$scope.filterReason = function(filter) {
		$location.search(filter);
		$scope.filtersModal = false;
	}

	$scope.filter = {
		source_se: validString($location.search().source_se),
		dest_se:   validString($location.search().dest_se),
	}

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.optimizer = Optimizer.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


OptimizerCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, Optimizer) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	Optimizer.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}


function OptimizerDetailedCtrl($location, $scope, optimizer, OptimizerDetailed)
{
	$scope.optimizer = optimizer;

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.optimizer = OptimizerDetailed.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Page
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
	
	// Set up filters
	$scope.filter = {
			source:      validString($location.search().source),
			destination: validString($location.search().destination)
	}
}


OptimizerDetailedCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, OptimizerDetailed) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	OptimizerDetailed.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
	}
}
