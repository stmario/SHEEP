### SHEEP frontend

This is a simple flask app, using WTForms to get input from the user,
then calling the `benchmark` executable to test a given HE library on a
user-specified circuit file.

### Requirements

```
pip install flask
pip install wtforms
pip install pytest
```

To run the app, from this directory, just do:
```
python app.py
```

