#!/usr/bin/env python
import sys
from twython import Twython
CONSUMER_KEY = 'sqv2GFAeSW9xQAdesweshThLQ'
CONSUMER_SECRET = ' ic36zhL1FzBT3zoYdEpP4r2lT9uqeR22lUl0Tb2o7yQcktG0Rb'
ACCESS_KEY = ' 3064474942-JUFSaWdOPlW0jk9t211GC4iXFnlLOkcdDmn41d2'
ACCESS_SECRET = ' SAvpfr9TDhY3yth2CP33uYoicrleBnWYf8I9LLENoNyYM'

api = Twython(CONSUMER_KEY,CONSUMER_SECRET,ACCESS_KEY,ACCESS_SECRET) 

api.update_status(status=sys.argv[1])

