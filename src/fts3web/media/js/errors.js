

function ErrorsCtrl($location, $scope, errors, Errors)
{
	// Errors
	$scope.errors = errors;
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.errors.page;
	$scope.pageCount = $scope.errors.pageCount;
	
	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
	
	// Set timer to trigger autorefresh
	$scope.autoRefresh = setInterval(function() {
		var filter = $location.search();
		filter.page = $scope.errors.page;
    	$scope.errors = Errors.query(filter);
	}, 20000);
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
    	
    	Errors.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
    }		
}



function FilesWithErrorCtrl($location, $scope, files, FilesWithError)
{
	$scope.reason = $location.search().reason;
	$scope.files = files
	
	// Paginator	
	$scope.pageMax   = 15;
	$scope.page      = $scope.files.page;
	$scope.pageCount = $scope.files.pageCount;
	
}



FilesWithErrorCtrl.resolve = {
    files: function($rootScope, $q, $location, FilesWithError) {
    	loading($rootScope);
    	
    	var deferred = $q.defer();

    	var page = $location.search().page;
    	if (!page || page < 1)
    		page = 1;
    	
    	FilesWithError.query($location.search(), function(data) {
    		deferred.resolve(data);
    		stopLoading($rootScope);
    	});
    	
    	return deferred.promise;
    }
}
