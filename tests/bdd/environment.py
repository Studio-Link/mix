def before_scenario(context, scenario):
    context.base_url = 'http://127.0.0.1:9999'
    context.ws = {}
    context.sessid = {}
    context.response = {}
