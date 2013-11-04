
function OptimizerCtrl($location, $scope, optimizer, Optimizer)
{
	$scope.optimizer = optimizer;
	
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
	}, REFRESH_INTERVAL);
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


function OptimizerDecisionsCtrl($location, $scope, decisions, OptimizerDecisions, Unique)
{
	$scope.decisions = decisions;
	
	// Unique pairs and vos
	$scope.unique = {
		sources: Unique('sources'),
		destinations: Unique('destinations')
	}
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.optimizer.page;
    	$scope.decisions = OptimizerDecisions.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
	
	// Set up filters
	$scope.filterBy = function(filter) {
		$location.search(filter);
	}
	
	$scope.filter = {
		source_se: undefinedAsEmpty($location.search().source_se),
		dest_se:   undefinedAsEmpty($location.search().dest_se)
	}
}


OptimizerDecisionsCtrl.resolve = {
	decisions: function($rootScope, $location, $route, $q, OptimizerDecisions) {
		loading($rootScope);
		
		var deferred = $q.defer();
		
		OptimizerDecisions.query($location.search(), function(data) {
			deferred.resolve(data);
			stopLoading($rootScope);
		});
		
		return deferred.promise;
	}
}
