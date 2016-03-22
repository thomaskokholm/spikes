const rpc_url = '/service/json'
const rest_base_url = '/service/entity'
const domain_name = 'test'
const rest_page_size = 24

// We all have to wait for ES6 iterators, in order to use for ... of structure ... sigth ...
interface Row<Data> {
    done: boolean; 
    idx: number; 
    data: Data;
}

interface Iter<Data> {
    total_count() : number;
    next() : Promise<Row<Data>>;
}

interface Entity<Key, Data> {
    key( data: Data ) : Key;
    get( key: Key ) : Promise<Data>;
    set( key: Key, data: Data ) : void;
    create( data: Data ) : Promise<Data>;
    remove( key: Key ) : void;
    
    query( args: any, order: string[] ) : Iter<Data>;
}

export function mk_query(params: any) : string {
    let qs = "";
    
    for(let key in params) {
        let value = params[key]
        
        if( value )
            qs += encodeURIComponent(key) + "=" + encodeURIComponent(value) + "&";
        else
            qs += encodeURIComponent(key)
    }
  
    return qs;
}

export function mk_url(url: string, params: any) : string {
    let qs = mk_query( params );
    
    if (qs.length > 0){
        qs = qs.substring(0, qs.length-1); //chop off last "&"
        url = `${url}?${qs}`;
    }
    
    return url;
}

var last_rpc_seq_id = 0
export function rpc( method: string, ...args: any[] ) : Promise<any> {
	return new Promise<any>((resolve, reject) => {	 
			var id = last_rpc_seq_id++;

			let envelope = {
				'jsonrpc': '2.0',
				'id': id,
				'method': method,
				'params': args
			};

			let h = new Headers();
			h.append('Content-Type', 'application/json')
			h.append('Essens-Domain', domain_name)
			
			// Time to use a propper request handler from ES6
			let conn = fetch( rpc_url, {
				method: 'POST',
				mode: 'same-origin',
				body: JSON.stringify( envelope ),
				cache: 'no-store',
				headers: h
			}).then((response) => {
				if(response.status != 200) {
					console.error( 'RPC HTTP error ', response.status, ':', response.statusText );
	
					reject({message: `RPC HTTP communication error ${response.status}`, code: -1})
				} else {
                     if (response.headers.get('Content-Type').match( 'application/json' )) {
                        response.json().then((res) => {
                            if( res[ 'id' ] == id ) {
                                if( 'error' in res ) {
                                    // An error in the logic, user need to take action
                                    console.error( 'RPC method error ', res[ 'error' ] )
                
                                    reject( res[ 'error' ] )
                                } else if( 'result' in res ) {
                                    resolve( res[ 'result' ] )
                                } else
                                    console.error( 'RPC unknown answer ', res )
                            } else {
                                // We should never end up here, and if we do the error need to be analized 
                                console.error( 'RPC protocol error for ', method, ' payload ', 
                                        JSON.stringify(envelope), ' result ', res )
                
                                reject({message: 'jsonrpc sequence mismatch in method ' + method, code: -1})
                            }
                        })
                     } else
                        reject({message: 'RPC response not json encoded, but ' + response.headers.get('Content-Type'), code: -1})
				}
			}).catch((error) => {
				// the backend is gone ...
				console.error( 'RPC comm error ', error )
	
				reject({message:error, code: -1})
			});
		} 
	)
}

/**
 * Provide a batch that act like normal rpc but it will only execute the rpc functions 
 * when the commit function are called and then pack all into one single json request batch
 * 
 * When a response returns the rpc functions will be notified as if the normal rpc function
 * has been used  
 * */ 
export class Batch {
    private payload: any[] = [];
    private last_id = 0;
    private ids: {[key:number]: {resolve: {(res: any) : void}, reject:{( err?: any ) : void} }} = {};
        
    public rpc( method: string, ...args: any[] ) : Promise<any> {
        let id = ++this.last_id
        
        let envelope = {
            'jsonrpc': '2.0',
            'id': id,
            'method': method,
            'params': args
        };
        
        this.payload.push( envelope )
        
        let p = new Promise<any>((resolve, reject) => {
            this.ids[ id ] = {
                resolve: resolve,
                reject: reject
            }
        })
        
        return p
    }

    public commit() : Promise<any> {
        return new Promise<any>((resolve, reject) => {
            let h = new Headers();
            h.append('Content-Type', 'application/json')
            h.append('Essens-Domain', domain_name)
                
            fetch( rpc_url, {
                method: 'POST',
                mode: 'same-origin',
                body: JSON.stringify( this.payload ),
                cache: 'no-store',
                headers: h
            }).then((response) => {
                if(response.status != 200) {
                    console.error( 'RPC HTTP error ', response.status, ' ', response.statusText )

                    reject({message: `RPC HTTP communication error ${response.status}`, code: -1})
                } else {
                    if (response.headers.get('Content-Type').match( 'application/json' )) {
                        response.json().then((res) => {
                            for( let r of res ) {
                                let id = parseInt( r[ 'id' ] )
                                
                                if( id in this.ids ) {
                                    let cur = this.ids[ id ]
                                    
                                    if( 'result' in res )             
                                        cur.resolve( res[ 'result' ])
                                    else
                                        cur.reject( res[ 'error' ])
                                }
                            }
                        })
                    }
               
                    // Reset states to make it possible use this instance as a new batch     
                    this.payload = []
                    this.ids = {}
                }
            })
        })        
    }
}

const items_reg = /items=(\d+)-(\d+)\/?(\d*)/
class RestIter<Data> implements Iter<Data> {
    private total = -1;
    private cur_idx = -1;
    private entity_name: string;
    private args: any;
    private order: string[];
    
    private url: string;
    private single_buf: Promise<Data[]>;
    private fetch_buf: {[key:string]:Promise<Data[]>};
    
    constructor( entity_name: string, args: any, order: string[] ) {
        this.entity_name = entity_name
        this.args = args
        this.order = order
        
        // need order too !!!
        args[ `sort(${order.toString()})` ] = undefined
        this.url = mk_url( `${rest_base_url}/${this.entity_name}`, args )
        
        this.reset()    
    }
    
    public reset() : void {
        this.cur_idx = 0
        this.fetch_buf = {}
        this.single_buf = null
    }
    
    public total_count() : number {
        return this.total
    }
    
    public next() : Promise<Row<Data>> {
        if( this.total != -1 && this.cur_idx >= this.total )
            return null
        
        let idx = ++this.cur_idx
        
        return new Promise<Row<Data>>((resolve, reject) => {
            // fetch buffer if none has been found before
            let begin = idx / rest_page_size
            let end = begin + rest_page_size
            let key = `${begin}-${end}`

            if( !(key in this.fetch_buf) || !this.single_buf ) {
                this.fetch_buf[ key ] = new Promise<Data[]>(( resolve, reject ) => {
                    let h = new Headers();
                    h.append('Essens-Domain', domain_name)
                    h.append('Range', `items=${begin}-${end}` )
                    
                    fetch( this.url, {
                        method: 'GET',
                        mode: 'same-origin',
                        cache: 'no-store',
                        headers: h
                    }).then((response) => {
                        let start = 0, end = 0, total = -1
                        
                        if( response.headers.has( 'Content-Range' )) {
                            let res = items_reg.exec( response.headers.get( 'Content-Range' ))
                        
                            if( res ) {
                                start = parseInt( res[ 0 ] ) 
                                end = parseInt( res[ 1 ] )
                                total = -1
                                
                                if( res.length == 4 ) {
                                    total = parseInt( res[ 2 ] )
                                    this.total = total
                                }
                            }
                        } else 
                            this.single_buf = this.fetch_buf[ key ];
                    
                        response.json().then( jdata => {
                            resolve(jdata);        
                        })  
                    })    
                })
            }
            
            if( this.single_buf ) {
                this.single_buf.then( data => {
                    resolve({done: true, idx: idx, data: data[ begin - idx ]})                     
                })
            } else {
                if( key in this.fetch_buf ) {
                    this.fetch_buf[ key ].then( data => {
                        resolve({done: true, idx: idx, data: data[ begin - idx ]})                     
                    })
                }   
            }    
        })
    }
}


/**
 * Define a rest entity store, that can handle normal RESTful json ations, and a query
 * that handle paged loading for large result set
 */
class RestEntity<Data> implements Entity<number, Data> {
    private entity_name: string 
    
    constructor( entity_name: string ) {
        this.entity_name = entity_name
    }
    
    private headers_get() : Headers {
        let h = new Headers();
        h.append('Essens-Domain', domain_name)
        
        return h;
    }
    
    private url_get( key?: any ) : string {
        let url = `${rest_base_url}/${this.entity_name}`
        
        if( key )
            url += `/${key}`
        
        return url
    }
    
    public key( data: Data ) : number {
        return data[ 'id' ];
    }
    
    public get( key: number ) : Promise<Data> {
        return new Promise<Data>((resolve, reject) => {
            fetch( this.url_get( key ), {
                method: 'GET',
                mode: 'same-origin',
                cache: 'no-store',
                headers: this.headers_get()
            }).then((res) => {
                res.json().then(jdata => {
                    resolve( jdata )
                })
            }).catch( err => {
                console.error( 'Restful GET error', err );
            })
        })
    }
    
    public set( key: number, data: Data ) : void {
        fetch( this.url_get( key ), {
            method: 'PUT',
            mode: 'same-origin',
            cache: 'no-store',
            body: JSON.stringify( data ),
            headers: this.headers_get()
        }).catch( err => {
            console.error( 'Restful PUT error', err );
        })
    }
    
    public create( data: Data ) : Promise<Data> {
        return new Promise<Data>((resolve, reject) => {
            fetch( this.url_get(), {
                method: 'POST',
                mode: 'same-origin',
                cache: 'no-store',
                body: JSON.stringify( data ),
                headers: this.headers_get()
            }).then((res) => {
                res.json().then(jdata => {
                    resolve( jdata )
                })
            }).catch( err => {
                console.error( 'Restful POST error', err );
            })
        })
    }
    
    public remove( key: number ) : void {
        fetch( this.url_get( key ), {
            method: 'DELETE',
            mode: 'same-origin',
            cache: 'no-store',
            headers: this.headers_get()
        }).catch( err => {
            console.error( 'Restful DELETE error', err );
        })
    }
    
    public query( args: any, order: string[] ) : Iter<Data> {
        return new RestIter<Data>( this.entity_name, args, order );
    }
}
