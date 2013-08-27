
function OptimizerCtrl($location, $scope, optimizer, Optimizer)
{
	$scope.optimizer = optimizer;
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.optimizer.page;
	$scope.pageCount = $scope.optimizer.pageCount;
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.optimizer = Optimizer.query(filter);
	}, 20000);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


OptimizerCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, Optimizer) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	Optimizer.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
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
	}, 20000);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filter = {
			source:      undefinedAsEmpty($location.search().source),
			destination: undefinedAsEmpty($location.search().destination)
	}
}


OptimizerDetailedCtrl.resolve = {
	optimizer: function ($rootScope, $location, $route, $q, OptimizerDetailed) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	OptimizerDetailed.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}

