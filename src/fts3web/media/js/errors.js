

function ErrorsCtrl($location, $scope, errors, Errors, Unique)
{
	// Errors
	$scope.errors = errors;
	
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
		contains:  validString($location.search().contains),
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
		filter.page = $scope.errors.page;
    	$scope.errors = Errors.query(filter);
	}, REFRESH_INTERVAL);
	$scope.$on('$destroy', function() {
		clearInterval($scope.autoRefresh);
	});
}


ErrorsCtrl.resolve = {
    errors: function($rootScope, $q, $location, Errors) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	Errors.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
    }		
}



function FilesWithErrorCtrl($location, $scope, files, FilesWithError)
{
	$scope.reason    = $location.search().reason;
	$scope.files     = files
	$scope.source_se = $location.search().source_se;
	$scope.dest_se   = $location.search().dest_se;
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
}



FilesWithErrorCtrl.resolve = {
    files: function($rootScope, $q, $location, FilesWithError) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	FilesWithError.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));
    	
    	return deferred.promise;
    }
}
