var path = require('path')

module.exports = {
  module: {
    rules: [
      {
        test: /encoderWorker\.min\.js$/,
        use: [{
          loader: 'file-loader',
        }]
      }
    ]
  },
  output: {
    filename: 'main.js',
    path: path.resolve(__dirname, 'dist')
  },
  mode: 'production'
};