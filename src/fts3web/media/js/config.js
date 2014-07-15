/// Config audit
function ConfigAuditCtrl($location, $scope, config, ConfigAudit)
{
	$scope.config = config;

	// Filter
	$scope.filter = {
		action:   validString($location.search().action),
		user:     validString($location.search().user),
		contains: validString($location.search().contains)
	};

	$scope.applyFilters = function() {
		$location.search({
			page:        1,
			action:   $scope.filter.action,
			user:     $scope.filter.user,
			contains: $scope.filter.contains
		});
	}

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

    $scope.showFilterDialog = function() {
    	document.getElementById('filterDialog').style.display = 'block';
    }
}

ConfigAuditCtrl.resolve = {
	config: function ($rootScope, $location, $route, $q, ConfigAudit) {
    	loading($rootScope);

    	var deferred = $q.defer();

    	ConfigAudit.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

    	return deferred.promise;
	}
}

/// Config status
function ConfigStatusCtrl($location, $scope, server, debug)
{
	$scope.server = server;
	$scope.debug = debug;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
}

ConfigStatusCtrl.resolve = {
	server: function ($rootScope, $location, $route, $q, ConfigServer) {
		loading($rootScope);

		var deferred = $q.defer();

		ConfigServer.all(
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

		return deferred.promise;
	},

	debug: function($rootScope, $location, $route, $q, ConfigDebug) {
		loading($rootScope);

		var deferred = $q.defer();

		ConfigDebug.query(
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

		return deferred.promise;
	}
}

/// Config links
function ConfigLinksCtrl($location, $scope, links) {
	$scope.links = links;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
}

ConfigLinksCtrl.resolve = {
	links: function($rootScope, $location, $route, $q, ConfigLinks) {
		loading($rootScope);

		var deferred = $q.defer();

		ConfigLinks.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

		return deferred.promise;
	},
}

/// Config limits
function ConfigLimitsCtrl($location, $scope, limits) {
	$scope.limits = limits;

	// On page change, reload
	$scope.pageChanged = function(newPage) {
		$location.search('page', newPage);
	};
}

ConfigLimitsCtrl.resolve = {
	limits: function($rootScope, $location, $route, $q, ConfigLimits) {
		loading($rootScope);

		var deferred = $q.defer();

		ConfigLimits.query($location.search(),
  			  genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

		return deferred.promise;
	}
}

/// Gfal2 configuration
function Gfal2Ctrl($location, $scope, gfal2) {
    $scope.gfal2 = gfal2;
}

Gfal2Ctrl.resolve = {
    gfal2: function($rootScope, $location, $q, ConfigGfal2) {
        loading($rootScope);

        var deferred = $q.defer();

        ConfigGfal2.query($location.search(),
              genericSuccessMethod(deferred, $rootScope),
			  genericFailureMethod(deferred, $rootScope, $location));

        return deferred.promise;
    }
}
