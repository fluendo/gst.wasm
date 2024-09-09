module.exports = {
    server: {
	middleware: {
	    // Add custom middleware to modify the response headers
	    1: (req, res, next) => {
		// Set the required headers
		res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
		res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
		next();
	    }
	},
	baseDir: '.',
	routes: {
	    '/medias': 'medias'
	}
    },
};
