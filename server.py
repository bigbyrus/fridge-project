from unicodedata import name
from flask import Flask, request
from flask_restful import Api, Resource, reqparse, abort
from flask_sqlalchemy import SQLAlchemy

app = Flask(__name__)
api = Api(app)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///database.db'
db = SQLAlchemy(app)

class ImageModel(db.model):
    #id has unique identifier bc of primary_key
    id = db.Column(db.integer, primary_key=True)
    #must have a name bc of nullable false
    name = db.Column(db.string, nullable = False)

    def __repr__(self):
        return f"Image(id={id}, name={name})"


db.create_all()


#parse through the request being sent, see if it has
#the correct info
image_put_args = reqparse.RequestParser()
image_put_args.add_argument("username", type=str, help="Username missing", required=True)
image_put_args.add_argument("date", type=str, help="Date missing", required=True)
image = {}



class Image(Resource):
    #Override get request
    def get(self, image_id):
        result = ImageModel.query.get(id=image_id)
        return image[image_id]
    
    def put(self, image_id):
        args = image_put_args.parse_args()
        image[image_id] = args
        #we dont have to return this
        #(helpful for the future)
        return image[image_id]
    
    def delete(self, image_id):
        abort_nonexistent(image_id)
        del image[image_id]
        return '', 204


#root i.e. @app_route
api.add_resource(Image, "image/<int:image_id>")

if __name__ == '__main__':
    #start_listener()
    app.run(host = "0.0.0.0", port=8000)
    ##python -m http.server 8000 --bind 0.0.0.0