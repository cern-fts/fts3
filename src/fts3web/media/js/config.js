
function ConfigCtrl($location, $scope, config, Configuration)
{
	$scope.config = config;
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.config.page;
	$scope.pageCount = $scope.config.pageCount;
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.config.page;
    	$scope.config = Configuration.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}

ConfigCtrl.resolve = {
	config: function ($rootScope, $location, $route, $q, Configuration) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	Configuration.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
	}
}
