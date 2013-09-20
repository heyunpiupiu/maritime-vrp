//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <subproblem/sp_solver.h>

inline bool solution_in_pool(const Solution& s, const ColumnPool& pool) {
    return (find_if(pool.begin(), pool.end(),
    [&s] (const Column& c) {
        return (c.sol == s);
    }) != pool.end());
}

inline void print_report(const int sols_found, const int discarded_prc, const int discarded_infeasible, const int discarded_generated, const int discarded_in_pool) {
    cout << "We found " << sols_found << " new columns." << endl;
    cout << "\t" << discarded_prc << " columns were discarded because they have positive reduced cost." << endl;
    cout << "\t" << discarded_infeasible << " columns were discarded because they're infeasible wrt capacity constraints." << endl;
    cout << "\t" << discarded_generated << " columns were discarded because they had already been generated in this iteration." << endl;
    cout << "\t" << discarded_in_pool << " columns were discarded because they were already in the columns pool." << endl;
}

bool SPSolver::solve(ColumnPool& pool) {
    vector<Solution> valid_sols;
    
    int discarded_prc = 0;
    int discarded_infeasible = 0;
    int discarded_generated = 0;
    int discarded_in_pool = 0;
    
    for(auto& vg : prob.graphs) {
        const Graph& g = vg.second;
        HeuristicsSolver hsolv(prob.params, g);
        vector<Solution> fast_fwd_sols = hsolv.solve_fast_forward();
        
        for(const Solution& s : fast_fwd_sols) {
            if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints(g)) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, pool)) {
                discarded_in_pool++;
            } else {
                valid_sols.push_back(s);
            }
        }
    }
    
    cout << "Fast forward heuristics." << endl;
    print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            pool.push_back(Column(prob, s));
        }
        return true;
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    for(auto& vg : prob.graphs) {
        const Graph& g = vg.second;
        HeuristicsSolver hsolv(prob.params, g);
        vector<Solution> fast_bwd_sols = hsolv.solve_fast_backward();
        
        for(const Solution& s : fast_bwd_sols) {
            if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints(g)) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, pool)) {
                discarded_in_pool++;
            } else {
                valid_sols.push_back(s);
            }
        }
    }
    
    cout << "Fast backward heuristics." << endl;
    print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            pool.push_back(Column(prob, s));
        }
        return true;
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    for(auto& vg : prob.graphs) {
        const Graph& g = vg.second;
        HeuristicsSolver hsolv(prob.params, g);
        float lambda = prob.params.lambda_start;
        
        while(valid_sols.size() == 0 && lambda <= prob.params.lambda_end) {
            vector<Solution> red_sols = hsolv.solve_on_reduced_graph(lambda);
        
            for(const Solution& s : red_sols) {
                if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                    discarded_prc++;
                } else if(!s.satisfies_capacity_constraints(g)) {
                    discarded_infeasible++;
                } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                    discarded_generated++;
                } else if(solution_in_pool(s, pool)) {
                    discarded_in_pool++;
                } else {
                    valid_sols.push_back(s);
                }
            }
            
            lambda += prob.params.lambda_inc;
        }
    }
    
    cout << "Labelling on the reduced graph." << endl;
    print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            pool.push_back(Column(prob, s));
        }
        return true;
    } else {
        discarded_prc = 0; discarded_infeasible = 0; discarded_generated = 0; discarded_in_pool = 0;
    }
    
    for(auto& vg : prob.graphs) {
        const Graph& g = vg.second;
        ExactSolver esolv(g);
        vector<Solution> e_sols = esolv.solve();
    
        for(const Solution& s : e_sols) {
            if(s.reduced_cost > -numeric_limits<float>::epsilon()) {
                discarded_prc++;
            } else if(!s.satisfies_capacity_constraints(g)) {
                discarded_infeasible++;
            } else if(find(valid_sols.begin(), valid_sols.end(), s) != valid_sols.end()) {
                discarded_generated++;
            } else if(solution_in_pool(s, pool)) {
                discarded_in_pool++;
            } else {
                valid_sols.push_back(s);
            }
        }
    }
    
    cout << "Labelling on the complete graph." << endl;
    print_report(valid_sols.size(), discarded_prc, discarded_infeasible, discarded_generated, discarded_in_pool);
    
    if(valid_sols.size() > 0) {
        for(const Solution& s : valid_sols) {
            pool.push_back(Column(prob, s));
        }
        return true;
    }
    
    return false;
}