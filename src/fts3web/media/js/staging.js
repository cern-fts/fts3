function StagingCtrl($location, $scope, staging, Staging)
{
	// Entries
	$scope.staging = staging;
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.staging.page;
	$scope.pageCount = $scope.staging.pageCount;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};

	// Set timer to trigger autorefresh
	$scope.filter = {
		page: 1
	}
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.staging.page;
    	$scope.staging = Staging.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


StagingCtrl.resolve = {
    staging: function($rootScope, $location, $q, Staging) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Staging.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
    }
}
