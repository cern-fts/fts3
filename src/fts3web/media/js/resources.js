angular.module('ftsmon.resources', ['ngResource'])
.factory('Job', function($resource) {
	return $resource('jobs/:jobId', {}, {
		query: {method: 'GET',
			    isArray: false},
	})
})
.factory('Unique', function($resource) {
	return $resource('unique/', {}, {
		all: {method: 'GET', isArray: false}
	})
});
