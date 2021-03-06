// This file is part of MLDB. Copyright 2015 mldb.ai inc. All rights reserved.

var mldb = require('mldb')
var unittest = require('mldb/unittest')

var dataset = mldb.createDataset({type:'sparse.mutable',id:'test'});

var ts = new Date("2015-01-01");

var row = 0;

function recordExample(who, what, how)
{
    dataset.recordRow(row++, [ [ "who", who, ts ], ["what", what, ts], ["how", how, ts] ]);
}

recordExample("mustard", "moved", "kitchen");
recordExample("plum", "moved", "kitchen");
recordExample("mustard", "stabbed", "plum");
recordExample("mustard", "killed", "plum");
recordExample("plum", "died", "stabbed");

dataset.commit()

var resp = mldb.get("/v1/query", {q: 'SELECT pivot(what, how) AS * NAMED who FROM test GROUP BY who ORDER BY who', format: 'sparse'});

plugin.log(resp.json);

unittest.assertEqual(resp.responseCode, 200, "Error executing query");

expected = [
   [
      [ "_rowName", "mustard" ],
      [ "killed", "plum" ],
      [ "moved", "kitchen" ],
      [ "stabbed", "plum" ]
   ],
   [
      [ "_rowName", "plum" ],
      [ "died", "stabbed" ],
      [ "moved", "kitchen" ]
   ]
];

unittest.assertEqual(mldb.diff(expected, resp.json, false /* strict */), {},
            "Query 2 output was not the same as expected output");

"success"
